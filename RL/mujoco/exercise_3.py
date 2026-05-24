"""
EXERCISE 3: Tune hyperparameters for MuJoCo
Goal: Apply MuJoCo-specific tuning and see the difference.

Tasks:
  3a. Create PPO with tuned hyperparameters (filled in for you)
  3b. Train for 500K-1M timesteps
  3c. Compare learning curve with Exercise 2
  3d. Experiment: try changing one hyperparameter at a time to see the effect

Install first:
    pip install stable-baselines3[extra] gymnasium[mujoco]
"""

from xml.parsers.expat import model

from xml.parsers.expat import model

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.callbacks import BaseCallback


class RewardTracker(BaseCallback):
    """Tracks episode rewards during training."""
    def __init__(self):
        super().__init__()
        self.episode_rewards = []
        self.episode_timesteps = []

    def _on_step(self) -> bool:
        for info in self.locals.get("infos", []):
            if "episode" in info:
                self.episode_rewards.append(info["episode"]["r"])
                self.episode_timesteps.append(self.num_timesteps)
        return True


def exercise_3():
    print("=" * 60)
    print("EXERCISE 3: Tuned hyperparameters")
    print("=" * 60)

    env = make_vec_env("HalfCheetah-v5", n_envs=4, seed=42)

    # 3a. Create PPO with MuJoCo-tuned settings
    #     These are the "known good" settings for MuJoCo locomotion.
    #     Understand WHY each one is different from CartPole defaults.
    model = PPO(
        "MlpPolicy",
        env,
        target_kl=0.03,
        # Collect more data per update — walking needs longer rollouts
        n_steps=2048,           # (CartPole used 128)
        # WHY: 128 steps ≈ 0.5 seconds of walking. Not enough to learn a gait.
        #      2048 steps ≈ 8 seconds — enough for several full strides.

        # More gradient updates per batch — continuous actions are smoother
        n_epochs=10,            # (CartPole used 4)
        # WHY: Continuous actions change gradually, so the clipping constraint
        #      is less tight. Safe to squeeze more learning from each batch.

        batch_size=64,          # Same as CartPole

        # PPO clipping — same as CartPole!
        clip_range=0.2,

        # Slightly higher learning rate
        learning_rate=3e-4,     # (CartPole used 2.5e-4)

        # Entropy coefficient — slightly lower for continuous
        ent_coef=0.0,           # (CartPole used 0.01)
        # WHY: Gaussian exploration is built into the std — don't need
        #      extra entropy bonus. Some people still use a small value.

        # Value function coefficient
        vf_coef=0.5,

        # GAE — same as CartPole
        gamma=0.99,
        gae_lambda=0.95,

        # Gradient clipping
        max_grad_norm=0.5,

        # Bigger networks for the harder task
        policy_kwargs=dict(
            net_arch=dict(pi=[256, 256], vf=[256, 256]),
            activation_fn=torch.nn.Tanh,
            log_std_init=-2.0,  # Start with smaller exploration (exp(-2) ≈ 0.14)
            # WHY: MuJoCo actions are in [-1, 1]. A std of 0.14 means the agent
            #      starts with moderate exploration instead of wild random torques.
        ),

        verbose=1,
        seed=42,
    )

    # 3b. Train with reward tracking
    # TODO: Set the timesteps. Start with 500_000, increase to 1_000_000 if you
    #       have time. HalfCheetah typically converges around 1M.
    tracker = RewardTracker()
    TIMESTEPS = 500_000  # TODO: Try 1_000_000 if you have the patience

    print(f"\nTraining for {TIMESTEPS:,} timesteps...")
    model.learn(total_timesteps=TIMESTEPS, callback=tracker)

    # 3c. Evaluate
    mean_reward, std_reward = evaluate_policy(model, model.get_env(), n_eval_episodes=10)
    print(f"\nTuned PPO after {TIMESTEPS:,} steps: {mean_reward:.1f} +/- {std_reward:.1f}")

    # 3d. Plot the learning curve (uncomment when ready)
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(tracker.episode_timesteps, tracker.episode_rewards, alpha=0.3, color="steelblue")
    window = 20
    if len(tracker.episode_rewards) >= window:
        smoothed = np.convolve(tracker.episode_rewards, np.ones(window)/window, mode="valid")
        ax.plot(tracker.episode_timesteps[window-1:], smoothed, color="darkblue", linewidth=2)
    ax.set_xlabel("Timesteps")
    ax.set_ylabel("Episode Reward")
    ax.set_title("PPO on HalfCheetah-v5 (Tuned)")
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig("halfcheetah_learning_curve.png", dpi=150)
    plt.close()

    model.save("halfcheetah_ppo_tuned")

    # EXPERIMENT: Try changing ONE thing at a time and retrain:
    # - What happens with n_steps=128 (CartPole default)?
    # - What happens with net_arch=[64, 64] (smaller network)?
    # - What happens with n_epochs=4 (fewer gradient steps)?
    # - What happens with learning_rate=1e-2 (too high)?
    # Write your observations as comments here:
    # TODO
    view_env = gym.make("HalfCheetah-v5", render_mode="human")
    obs, _ = view_env.reset()

    for _ in range(1000):
        action, _ = model.predict(obs, deterministic=True)
        obs, reward, terminated, truncated, _ = view_env.step(action)
        if terminated or truncated:
            obs, _ = view_env.reset()

    view_env.close()
    return model


if __name__ == "__main__":
    exercise_3()
