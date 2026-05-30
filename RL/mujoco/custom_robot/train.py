"""
train.py — Train the custom SimpleBipedEnv with PPO.

Now that you own the robot body (XML) and the environment (runner_env.py),
this script is identical in structure to train_humanoid.py — the only
difference is which environment class we hand to PPO.

That's the point: PPO doesn't care about your robot. It just needs an env
that speaks the gymnasium API (obs, reward, terminated, truncated, info).

Usage:
    python RL/mujoco/custom_robot/train.py
    python RL/mujoco/custom_robot/train.py --watch --model runs/simple_biped/final_model.zip
"""

from __future__ import annotations

import sys
import argparse
from pathlib import Path

# Make sure runner_env.py is importable regardless of working directory.
sys.path.insert(0, str(Path(__file__).parent))
from runner_env import SimpleBipedEnv

import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import CheckpointCallback, EvalCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecNormalize

SEED = 42
RUN_DIR = Path("runs/simple_biped")


def make_env(render_mode: str | None = None):
    """Factory function: returns one instance of our custom env."""
    return SimpleBipedEnv(
        frame_skip=4,
        forward_reward_weight=2.5,
        ctrl_cost_weight=0.001,
        healthy_reward=1.0,
        target_speed=3.0,      # aim for 3 m/s running
        render_mode=render_mode,
    )


def train(timesteps: int, n_envs: int, run_dir: Path) -> None:
    run_dir.mkdir(parents=True, exist_ok=True)

    # make_vec_env calls make_env() n_envs times to get parallel copies.
    env = make_vec_env(make_env, n_envs=n_envs, seed=SEED)
    env = VecNormalize(env, norm_obs=True, norm_reward=True, clip_obs=10.0)

    eval_env = make_vec_env(make_env, n_envs=1, seed=SEED + 1)
    eval_env = VecNormalize(eval_env, training=False, norm_obs=True, norm_reward=False, clip_obs=10.0)
    eval_env.obs_rms = env.obs_rms

    model = PPO(
        "MlpPolicy",
        env,
        n_steps=2048,
        batch_size=4096,
        n_epochs=10,
        gamma=0.99,
        gae_lambda=0.95,
        learning_rate=3e-4,
        clip_range=0.2,
        ent_coef=0.0,
        vf_coef=0.5,
        max_grad_norm=0.5,
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

    callbacks = [
        CheckpointCallback(
            save_freq=max(10_000 // n_envs, 1),
            save_path=str(run_dir / "checkpoints"),
            name_prefix="simple_biped",
            save_vecnormalize=True,
        ),
        EvalCallback(
            eval_env,
            best_model_save_path=str(run_dir / "best"),
            log_path=str(run_dir / "eval"),
            eval_freq=max(50_000 // n_envs, 1),
            n_eval_episodes=5,
            deterministic=True,
        ),
    ]

    model.learn(total_timesteps=timesteps, callback=callbacks, progress_bar=True)
    model.save(run_dir / "final_model")
    env.save(run_dir / "vecnormalize.pkl")

    mean_reward, std_reward = evaluate_policy(model, eval_env, n_eval_episodes=10, deterministic=True)
    print(f"\nFinal eval: {mean_reward:.1f} +/- {std_reward:.1f}")
    env.close()
    eval_env.close()


def watch(model_path: Path, stats_path: Path) -> None:
    env = make_vec_env(lambda: make_env(render_mode="human"), n_envs=1, seed=SEED)
    if stats_path.exists():
        env = VecNormalize.load(stats_path, env)
        env.training = False
        env.norm_reward = False
    model = PPO.load(model_path)
    obs = env.reset()
    while True:
        action, _ = model.predict(obs, deterministic=True)
        obs, _, dones, _ = env.step(action)
        if dones[0]:
            obs = env.reset()


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--timesteps", type=int, default=2_000_000)
    parser.add_argument("--n-envs", type=int, default=8)
    parser.add_argument("--run-dir", type=Path, default=RUN_DIR)
    parser.add_argument("--watch", action="store_true")
    parser.add_argument("--model", type=Path, default=RUN_DIR / "final_model.zip")
    parser.add_argument("--stats", type=Path, default=RUN_DIR / "vecnormalize.pkl")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    if args.watch:
        watch(args.model, args.stats)
    else:
        train(args.timesteps, args.n_envs, args.run_dir)
