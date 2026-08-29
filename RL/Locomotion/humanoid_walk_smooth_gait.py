from __future__ import annotations

from pathlib import Path

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import CheckpointCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import VecNormalize


RUN_DIR = Path("runs/simple_humanoid_smooth_gait_walk")
SEED = 42
MODE = "watch"  # Change to "watch" after training.
TIMESTEPS = 2_000_000
N_ENVS = 8
MODEL_PATH = RUN_DIR / "final_model.zip"
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
    def __init__(self, env, contact_weight=0.3, upright_weight=0.2, gait_cycle_steps=80):
        super().__init__(env)
        self.contact_weight = contact_weight
        self.upright_weight = upright_weight
        self.gait_cycle_steps = gait_cycle_steps
        self.phase = 0.0

        model = self.unwrapped.model
        self.floor_geom_id = model.geom("floor").id
        self.right_foot_geom_id = model.geom("right_foot").id
        self.left_foot_geom_id = model.geom("left_foot").id
        self.torso_body_id = model.body("torso").id

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

        reward += self.contact_weight * contact_reward
        reward += self.upright_weight * upright_reward

        self.phase = (self.phase + 1.0 / self.gait_cycle_steps) % 1.0

        info["custom/gait_phase"] = self.phase
        info["custom/contact_reward"] = contact_reward
        info["custom/upright_reward"] = upright_reward
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


def make_env(render_mode: str | None = None):
    env = gym.make("Humanoid-v5", render_mode=render_mode, default_camera_config=TRACK_CAMERA_CONFIG)
    env = SmoothReward(env, weight=0.08)
    env = GaitTimingReward(env, contact_weight=0.3, upright_weight=0.2)
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
