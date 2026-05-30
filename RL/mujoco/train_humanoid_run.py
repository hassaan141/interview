"""
Train a MuJoCo humanoid to RUN with PPO.

CONCEPT: Behavior in RL is entirely shaped by the reward function.
  - Walking emerges at ~1-2 m/s with the default Humanoid-v5 reward.
  - Running (~3-5 m/s) requires "flight phases" — moments where both feet
    leave the ground. That involves larger joint torques and harder landings,
    which the DEFAULT reward penalises via ctrl_cost and contact_cost.
  - To get running, you must tip the balance: reward speed more, penalise
    impact/effort less.

TWO LEVERS (both shown here):
  1. Tune the built-in env reward weights (simplest, no extra code).
  2. Wrap the env and add a velocity-target bonus (cleaner signal).

Usage:
    python RL/mujoco/train_humanoid_run.py --timesteps 2000000
    python RL/mujoco/train_humanoid_run.py --watch --model runs/humanoid_run/final_model.zip
"""

from __future__ import annotations

import argparse
from pathlib import Path

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import CheckpointCallback, EvalCallback
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.vec_env import VecNormalize


SEED = 42
RUN_DIR = Path("runs/humanoid_run")

# ──────────────────────────────────────────────────────────────────────────────
# STEP 1 — Understand the default reward
# ──────────────────────────────────────────────────────────────────────────────
#
# Humanoid-v5 total reward (per timestep):
#
#   R = forward_reward_weight * x_velocity   ← want to go forward
#     + healthy_reward                        ← want to stay upright (5.0/step)
#     - ctrl_cost_weight   * ||action||²      ← punish large joint torques
#     - contact_cost_weight * ||forces||²     ← punish hard foot contacts
#
# Default weights:
#   forward_reward_weight = 1.25
#   healthy_reward        = 5.0  (fixed, keep it)
#   ctrl_cost_weight      = 0.1
#   contact_cost_weight   = 5e-7
#
# Problem: at 3 m/s the control effort (~0.1 * effort²) and contact costs
# are large enough to make slow-but-stable walking more profitable than fast
# running. The agent rationally chooses to walk.
#
# Fix: increase the forward reward weight, and reduce the cost weights so
# that running becomes the highest-value strategy.

RUNNING_ENV_KWARGS = dict(
    forward_reward_weight=2.5,   # was 1.25 — speed is worth twice as much
    ctrl_cost_weight=0.05,       # was 0.10 — half the torque penalty
    contact_cost_weight=1e-7,    # was 5e-7 — running has hard landings; allow them
    # healthy_reward stays at 5.0 — still want the agent to stay upright
)


# ──────────────────────────────────────────────────────────────────────────────
# STEP 2 — Optional velocity-target wrapper
# ──────────────────────────────────────────────────────────────────────────────
#
# The env weights above make "faster = better" but give no ceiling. If you
# want the agent to aim for a SPECIFIC speed (e.g. 3 m/s jog), use this
# wrapper to add a shaped bonus on top of the env's own reward.
#
# The bonus uses a quadratic "well": maximum at the target speed, falling off
# on either side. This prevents the agent from just sprinting recklessly.
#
#   bonus = bonus_scale * max(0, 1 - ((v - target) / tolerance)²)
#
# At v = target:    bonus = bonus_scale   (full reward)
# At v = target ± tolerance: bonus ≈ 0
# Below target:     bonus tapers down   (still some incentive to speed up)

class VelocityTargetWrapper(gym.Wrapper):
    """Adds a shaped bonus for hitting a target forward speed."""

    def __init__(self, env: gym.Env, target_speed: float = 3.0, tolerance: float = 1.0, bonus_scale: float = 1.0):
        super().__init__(env)
        self.target_speed = target_speed
        self.tolerance = tolerance
        self.bonus_scale = bonus_scale

    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)

        # MuJoCo puts the forward velocity in the info dict
        x_velocity = info.get("x_velocity", 0.0)

        # Quadratic bonus — peaks at target_speed, zero outside ±tolerance
        deviation = (x_velocity - self.target_speed) / self.tolerance
        velocity_bonus = self.bonus_scale * max(0.0, 1.0 - deviation ** 2)

        reward = reward + velocity_bonus
        return obs, reward, terminated, truncated, info


def make_running_env(n_envs: int, seed: int, render_mode: str | None = None, use_velocity_wrapper: bool = True):
    """Build vectorised running envs with the reward adjustments applied."""

    def make_env():
        kwargs = dict(**RUNNING_ENV_KWARGS)
        if render_mode:
            kwargs["render_mode"] = render_mode
        env = gym.make("Humanoid-v5", **kwargs)
        if use_velocity_wrapper:
            env = VelocityTargetWrapper(env, target_speed=3.0, tolerance=1.0, bonus_scale=1.0)
        return env

    return make_vec_env(make_env, n_envs=n_envs, seed=seed)


# ──────────────────────────────────────────────────────────────────────────────
# STEP 3 — Network and PPO hyperparameter changes for running
# ──────────────────────────────────────────────────────────────────────────────
#
# The walk script used log_std_init=-2.0 (std ≈ 0.14).
# Running requires the agent to discover bigger, more energetic joint motions.
# A slightly higher initial std (-1.5 → std ≈ 0.22) gives more exploration
# early on so the agent can stumble across faster gaits before settling down.
#
# Everything else stays the same as the walk script — PPO hyperparameters
# for locomotion are well-established and don't need to change just because
# the target speed increased.

def make_model(env, run_dir: Path) -> PPO:
    return PPO(
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
            log_std_init=-1.5,   # slightly more exploration than walk (-2.0)
        ),
        tensorboard_log=str(run_dir / "tb"),
        verbose=1,
        seed=SEED,
    )


# ──────────────────────────────────────────────────────────────────────────────
# Training and watching — same structure as train_humanoid.py
# ──────────────────────────────────────────────────────────────────────────────

def train(timesteps: int, n_envs: int, run_dir: Path) -> None:
    run_dir.mkdir(parents=True, exist_ok=True)

    env = make_running_env(n_envs, seed=SEED, use_velocity_wrapper=True)
    env = VecNormalize(env, norm_obs=True, norm_reward=True, clip_obs=10.0)

    eval_env = make_running_env(1, seed=SEED + 1, use_velocity_wrapper=True)
    eval_env = VecNormalize(eval_env, training=False, norm_obs=True, norm_reward=False, clip_obs=10.0)
    eval_env.obs_rms = env.obs_rms

    model = make_model(env, run_dir)

    callbacks = [
        CheckpointCallback(
            save_freq=max(10_000 // n_envs, 1),
            save_path=str(run_dir / "checkpoints"),
            name_prefix="humanoid_run",
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

    print("\nWhat to watch in TensorBoard:")
    print("  rollout/ep_rew_mean  — should climb steadily (target > 6000)")
    print("  train/approx_kl      — should stay near 0.03 (your target_kl)")
    print("  train/explained_variance — should approach 1.0 as critic improves\n")

    model.learn(total_timesteps=timesteps, callback=callbacks, progress_bar=True)

    model.save(run_dir / "final_model")
    env.save(run_dir / "vecnormalize.pkl")

    mean_reward, std_reward = evaluate_policy(model, eval_env, n_eval_episodes=10, deterministic=True)
    print(f"\nFinal eval: {mean_reward:.1f} +/- {std_reward:.1f}")

    env.close()
    eval_env.close()


def watch(model_path: Path, stats_path: Path) -> None:
    env = make_running_env(1, seed=SEED, render_mode="human", use_velocity_wrapper=True)
    if stats_path.exists():
        env = VecNormalize.load(stats_path, env)
        env.training = False
        env.norm_reward = False
    else:
        print(f"Warning: {stats_path} not found; watching without normalisation stats.")
    model = PPO.load(model_path)
    obs = env.reset()

    while True:
        action, _ = model.predict(obs, deterministic=True)
        obs, _, dones, _ = env.step(action)
        if dones[0]:
            obs = env.reset()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--timesteps", type=int, default=2_000_000,
                        help="Running is harder than walking; 2M is a good starting point")
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
