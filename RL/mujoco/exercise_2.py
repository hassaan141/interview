"""
EXERCISE 2: Train with default hyperparameters
Goal: Get a baseline training run. See how far default settings get you.

Tasks:
  2a. Create vectorized environments (4 parallel)
  2b. Create a PPO model with "MlpPolicy" and defaults
  2c. Train for 100K timesteps (fast, won't fully converge)
  2d. Evaluate and compare to random baseline from Ex 1

Install first:
    pip install stable-baselines3[extra] gymnasium[mujoco]
"""

from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy


def exercise_2():
    print("=" * 60)
    print("EXERCISE 2: Train with defaults")
    print("=" * 60)

    # 2a. Create vectorized environments
    # HINT: make_vec_env("HalfCheetah-v5", n_envs=4, seed=42)
    env = None  # TODO

    # 2b. Create PPO model with default settings
    # HINT: PPO("MlpPolicy", env, verbose=1, seed=42)
    model = None  # TODO

    # 2c. Train for 100K steps (quick test — real training needs ~1M)
    # HINT: model.learn(total_timesteps=100_000)
    # TODO

    # 2d. Evaluate
    # HINT: evaluate_policy(model, model.get_env(), n_eval_episodes=10)
    # mean_reward, std_reward = TODO
    # print(f"Default PPO after 100K steps: {mean_reward:.1f} +/- {std_reward:.1f}")

    # Question: Is 100K enough? How does this compare to random?
    # A: TODO

    return model  # Keep for Exercise 4


if __name__ == "__main__":
    exercise_2()
