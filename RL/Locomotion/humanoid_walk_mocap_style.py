from __future__ import annotations

from pathlib import Path

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import CheckpointCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import VecNormalize


RUN_DIR = Path("runs/simple_humanoid_mocap_style_walk")
SEED = 42
MODE = "watch"  # Change to "watch" after training.
TIMESTEPS = 2_000_000
N_ENVS = 8
MODEL_PATH = RUN_DIR / "final_model"
STATS_PATH = RUN_DIR / "vecnormalize.pkl"
TRACK_CAMERA_CONFIG = dict(type=1, trackbodyid=1, distance=4.0, elevation=-20.0, azimuth=90.0)


class SmoothReward(gym.Wrapper):
    def __init__(self, env, weight=0.08):
        super().__init__(env)
        self.weight = weight
        self.prev_action = np.zeros(env.action_space.shape, dtype=np.float32)

    def reset(self, **kwargs):
        self.prev_action[:] = 0.0
        return self.env.reset(**kwargs)

    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)

        smoothness_cost = np.square(action - self.prev_action).mean()
        reward = reward - self.weight * smoothness_cost
        self.prev_action = action.copy()

        info["custom/smoothness_cost"] = smoothness_cost
        return obs, reward, terminated, truncated, info


class GaitTimingReward(gym.Wrapper):
    def __init__(
        self,
        env,
        contact_weight=0.3,
        upright_weight=0.2,
        step_weight=0.5,
        gait_cycle_steps=80,
    ):
        super().__init__(env)
        self.contact_weight = contact_weight
        self.upright_weight = upright_weight
        self.step_weight = step_weight
        self.gait_cycle_steps = gait_cycle_steps
        self.phase = 0.0

        model = self.unwrapped.model
        self.floor_geom_id = model.geom("floor").id
        self.right_foot_geom_id = model.geom("right_foot").id
        self.left_foot_geom_id = model.geom("left_foot").id
        self.torso_body_id = model.body("torso").id
        self.pelvis_body_id = model.body("pelvis").id
        self.right_foot_body_id = model.body("right_foot").id
        self.left_foot_body_id = model.body("left_foot").id

        low = np.concatenate([self.observation_space.low, np.array([-1.0, -1.0])])
        high = np.concatenate([self.observation_space.high, np.array([1.0, 1.0])])
        self.observation_space = gym.spaces.Box(low=low, high=high, dtype=np.float64)

    def reset(self, **kwargs):
        self.phase = 0.0
        obs, info = self.env.reset(**kwargs)
        return self._add_clock(obs), info

    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)

        contact_reward = self._contact_reward()
        upright_reward = self._upright_reward()
        step_reward = self._step_reward()

        reward += self.contact_weight * contact_reward
        reward += self.upright_weight * upright_reward
        reward += self.step_weight * step_reward

        self.phase = (self.phase + 1.0 / self.gait_cycle_steps) % 1.0

        info["custom/gait_phase"] = self.phase
        info["custom/contact_reward"] = contact_reward
        info["custom/upright_reward"] = upright_reward
        info["custom/step_reward"] = step_reward
        return self._add_clock(obs), reward, terminated, truncated, info

    def _add_clock(self, obs):
        angle = 2.0 * np.pi * self.phase
        clock = np.array([np.sin(angle), np.cos(angle)], dtype=obs.dtype)
        return np.concatenate([obs, clock])

    def _contact_reward(self):
        left_contact = self._foot_touching_floor(self.left_foot_geom_id)
        right_contact = self._foot_touching_floor(self.right_foot_geom_id)

        if self.phase < 0.5:
            expected_left_contact = True
            expected_right_contact = False
        else:
            expected_left_contact = False
            expected_right_contact = True

        left_match = left_contact == expected_left_contact
        right_match = right_contact == expected_right_contact
        return 0.5 * float(left_match) + 0.5 * float(right_match)

    def _foot_touching_floor(self, foot_geom_id):
        data = self.unwrapped.data
        for i in range(data.ncon):
            contact = data.contact[i]
            geom1 = contact.geom1
            geom2 = contact.geom2
            foot_floor = geom1 == foot_geom_id and geom2 == self.floor_geom_id
            floor_foot = geom1 == self.floor_geom_id and geom2 == foot_geom_id
            if foot_floor or floor_foot:
                return True
        return False

    def _upright_reward(self):
        mat = self.unwrapped.data.xmat[self.torso_body_id].reshape(3, 3)
        torso_up_dot_world_up = mat[2, 2]
        return max(float(torso_up_dot_world_up), 0.0)

    def _step_reward(self):
        data = self.unwrapped.data
        pelvis_x = data.xpos[self.pelvis_body_id][0]
        right_foot_x = data.xpos[self.right_foot_body_id][0]
        left_foot_x = data.xpos[self.left_foot_body_id][0]

        if self.phase < 0.5:
            swing_foot_ahead = right_foot_x - pelvis_x
        else:
            swing_foot_ahead = left_foot_x - pelvis_x

        return min(max(float(swing_foot_ahead), 0.0), 0.5)


class MocapStyleReward(gym.Wrapper):
    """Hand-mapped imitation reward inspired by the Naruto GLB.

    This is not full mocap retargeting. It uses the mocap as style guidance:
    forward torso lean, both arms stretched behind, and alternating leg pose.
    """

    def __init__(self, env, weight=0.6):
        super().__init__(env)
        self.weight = weight

        model = self.unwrapped.model
        self.torso_body_id = model.body("torso").id
        self.pelvis_body_id = model.body("pelvis").id
        self.right_upper_arm_body_id = model.body("right_upper_arm").id
        self.left_upper_arm_body_id = model.body("left_upper_arm").id
        self.right_hand_geom_id = model.geom("right_hand").id
        self.left_hand_geom_id = model.geom("left_hand").id

        self.joint_qpos = {
            name: model.jnt_qposadr[model.joint(name).id]
            for name in [
                "right_hip_y",
                "right_knee",
                "left_hip_y",
                "left_knee",
                "right_shoulder1",
                "right_shoulder2",
                "right_elbow",
                "left_shoulder1",
                "left_shoulder2",
                "left_elbow",
            ]
        }

    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)

        phase = float(info.get("custom/gait_phase", 0.0))
        style_reward = self._style_reward(phase)
        reward += self.weight * style_reward

        info["custom/mocap_style_reward"] = style_reward
        return obs, reward, terminated, truncated, info

    def _style_reward(self, phase):
        return (
            0.35 * self._arms_back_reward()
            + 0.25 * self._forward_lean_reward()
            + 0.40 * self._leg_reference_reward(phase)
        )

    def _arms_back_reward(self):
        data = self.unwrapped.data
        torso_x = data.xpos[self.torso_body_id][0]
        right_shoulder = data.xpos[self.right_upper_arm_body_id]
        left_shoulder = data.xpos[self.left_upper_arm_body_id]
        right_hand = data.geom_xpos[self.right_hand_geom_id]
        left_hand = data.geom_xpos[self.left_hand_geom_id]

        right_back = max(float(torso_x - right_hand[0]), 0.0)
        left_back = max(float(torso_x - left_hand[0]), 0.0)
        right_back_score = min(right_back / 0.45, 1.0)
        left_back_score = min(left_back / 0.45, 1.0)

        right_stretch_score = min(float(np.linalg.norm(right_hand - right_shoulder) / 0.45), 1.0)
        left_stretch_score = min(float(np.linalg.norm(left_hand - left_shoulder) / 0.45), 1.0)

        return min(right_back_score * right_stretch_score, left_back_score * left_stretch_score)

    def _forward_lean_reward(self):
        data = self.unwrapped.data
        torso_x = data.xpos[self.torso_body_id][0]
        pelvis_x = data.xpos[self.pelvis_body_id][0]
        forward_lean = max(float(torso_x - pelvis_x), 0.0)
        return float(np.exp(-20.0 * (forward_lean - 0.25) ** 2))

    def _leg_reference_reward(self, phase):
        data = self.unwrapped.data
        qpos = data.qpos

        if phase < 0.5:
            right_hip_target = -0.35
            right_knee_target = 0.45
            left_hip_target = 0.20
            left_knee_target = 0.15
        else:
            right_hip_target = 0.20
            right_knee_target = 0.15
            left_hip_target = -0.35
            left_knee_target = 0.45

        targets = {
            "right_hip_y": right_hip_target,
            "right_knee": right_knee_target,
            "left_hip_y": left_hip_target,
            "left_knee": left_knee_target,
            "right_shoulder1": -0.45,
            "right_shoulder2": -0.30,
            "right_elbow": -0.25,
            "left_shoulder1": 0.45,
            "left_shoulder2": 0.30,
            "left_elbow": -0.25,
        }

        error = 0.0
        for joint_name, target in targets.items():
            joint_value = qpos[self.joint_qpos[joint_name]]
            error += float((joint_value - target) ** 2)

        return float(np.exp(-2.0 * error))


def make_env(render_mode: str | None = None):
    env = gym.make("Humanoid-v5", render_mode=render_mode, default_camera_config=TRACK_CAMERA_CONFIG)
    env = SmoothReward(env, weight=0.08)
    env = GaitTimingReward(env, contact_weight=0.3, upright_weight=0.2, step_weight=0.5, gait_cycle_steps=105)
    env = MocapStyleReward(env, weight=0.6)
    return env


def make_model(env, run_dir: Path) -> PPO:
    return PPO(
        "MlpPolicy",
        env,
        n_steps=512,
        batch_size=1024,
        n_epochs=5,
        gamma=0.99,
        gae_lambda=0.95,
        learning_rate=3e-4,
        clip_range=0.2,
        target_kl=0.03,
        policy_kwargs=dict(
            net_arch=dict(pi=[256, 256], vf=[256, 256]),
            activation_fn=torch.nn.Tanh,
            log_std_init=-1.5,
        ),
        tensorboard_log=str(run_dir / "tb"),
        verbose=1,
        seed=SEED,
    )


def train() -> None:
    run_dir = RUN_DIR
    run_dir.mkdir(parents=True, exist_ok=True)

    env = make_vec_env(make_env, n_envs=N_ENVS, seed=SEED)
    env = VecNormalize(env, norm_obs=True, norm_reward=True, clip_obs=10.0)

    callback = CheckpointCallback(
        save_freq=max(50_000 // N_ENVS, 1),
        save_path=str(run_dir / "checkpoints"),
        name_prefix="humanoid_walk",
        save_vecnormalize=True,
    )

    model = make_model(env, run_dir)
    model.learn(total_timesteps=TIMESTEPS, callback=callback, progress_bar=True)

    model.save(run_dir / "final_model")
    env.save(run_dir / "vecnormalize.pkl")
    env.close()
    print(f"Saved model to {run_dir / 'final_model.zip'}")


def watch() -> None:
    env = make_vec_env(lambda: make_env(render_mode="human"), n_envs=1, seed=SEED)

    if STATS_PATH.exists():
        env = VecNormalize.load(STATS_PATH, env)
        env.training = False
        env.norm_reward = False

    model = PPO.load(MODEL_PATH)
    obs = env.reset()

    while True:
        action, _ = model.predict(obs, deterministic=True)
        obs, _, done, _ = env.step(action)
        if done[0]:
            obs = env.reset()


if __name__ == "__main__":
    if MODE == "watch":
        watch()
    else:
        train()
