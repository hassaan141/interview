"""
EXERCISE 5: Scale up to harder robots
Goal: Apply everything you've learned to Walker2d and Ant.
      After this, you can train any MuJoCo robot from scratch.

Tasks:
  5a. Train Walker2d-v5 (2M timesteps recommended)
  5b. Train Ant-v5 (5M timesteps recommended, bigger network)
  5c. Compare what makes each robot harder

Install first:
    pip install stable-baselines3[extra] gymnasium[mujoco]

Environments reference:
    HalfCheetah-v5  — 17 obs, 6 actions  — runs forward (2D)
    Walker2d-v5     — 17 obs, 6 actions  — bipedal walking (2D)
    Hopper-v5       — 11 obs, 3 actions  — one-legged hopping (2D)
    Ant-v5          — 27 obs, 8 actions  — four-legged walking (3D)
    Humanoid-v5     — 376 obs, 17 actions — full humanoid (3D)
"""
import gymnasium as gym
from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy

SEED = 42

def exercise_5():
    print("=" * 60)
    print("EXERCISE 5: Scale up")
    print("=" * 60)

    env = make_vec_env("Humanoid-v5", n_envs=32, seed=SEED)
    view_env = gym.make("Humanoid-v5", render_mode="human")

    model0=PPO(
        "MlpPolicy", 
        env, 
        verbose=1, 
        seed=SEED
        )

if __name__ == "__main__":
    exercise_5()
