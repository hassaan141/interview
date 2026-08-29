

from __future__ import annotations

from pathlib import Path

import gymnasium as gym
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import CheckpointCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import VecNormalize


RUN_DIR = Path("runs/simple_humanoid_walk")
SEED = 42
MODE = "watch"  # Change to "watch" after training.
TIMESTEPS = 2_000_000
N_ENVS = 8
MODEL_PATH = RUN_DIR / "final_model.zip"
STATS_PATH = RUN_DIR / "vecnormalize.pkl"
TRACK_CAMERA_CONFIG = dict(type=1, trackbodyid=1, distance=4.0, elevation=-20.0, azimuth=90.0)


def make_env(render_mode: str | None = None):
    return gym.make("Humanoid-v5", render_mode=render_mode, default_camera_config=TRACK_CAMERA_CONFIG)


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
    timesteps = TIMESTEPS
    n_envs = N_ENVS
    run_dir = RUN_DIR

    run_dir.mkdir(parents=True, exist_ok=True)

    env = make_vec_env(make_env, n_envs=n_envs, seed=SEED)
    env = VecNormalize(env, norm_obs=True, norm_reward=True, clip_obs=10.0)

    callback = CheckpointCallback(
        save_freq=max(50_000 // n_envs, 1),
        save_path=str(run_dir / "checkpoints"),
        name_prefix="humanoid_walk",
        save_vecnormalize=True,
    )

    model = make_model(env, run_dir)
    model.learn(total_timesteps=timesteps, callback=callback, progress_bar=True)

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
