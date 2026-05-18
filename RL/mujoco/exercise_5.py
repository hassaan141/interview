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

from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy


def exercise_5():
    print("=" * 60)
    print("EXERCISE 5: Scale up")
    print("=" * 60)

    # 5a. Walker2d — bipedal walking (must balance AND move)
    # TODO: Fill in the hyperparameters. Start with the HalfCheetah settings
    #       from Exercise 3, then adjust.
    # HINTS:
    #   - Walker2d needs more training time (2M+ timesteps)
    #   - Keep net_arch at [256, 256]
    #   - You might want to increase n_steps to 4096

    # walker_env = make_vec_env("Walker2d-v5", n_envs=4, seed=42)
    # walker_model = PPO(
    #     "MlpPolicy",
    #     walker_env,
    #     # TODO: Fill in hyperparameters
    #     verbose=1,
    #     seed=42,
    # )
    # walker_model.learn(total_timesteps=2_000_000)
    # mean_r, std_r = evaluate_policy(walker_model, walker_env, n_eval_episodes=10)
    # print(f"Walker2d: {mean_r:.1f} +/- {std_r:.1f}")

    # 5b. Ant — four-legged, 3D coordination
    # TODO: Fill in hyperparameters.
    # HINTS:
    #   - Ant needs even more training (5M+ timesteps)
    #   - Consider net_arch=[512, 256] for the larger observation space
    #   - gamma=0.99 is usually fine
    #   - This will take a while — consider using n_envs=8

    # ant_env = make_vec_env("Ant-v5", n_envs=8, seed=42)
    # ant_model = PPO(
    #     "MlpPolicy",
    #     ant_env,
    #     # TODO: Fill in hyperparameters
    #     verbose=1,
    #     seed=42,
    # )
    # ant_model.learn(total_timesteps=5_000_000)

    # 5c. Questions:
    # Q: What reward does Walker2d achieve? How does it compare to HalfCheetah?
    # A: TODO
    #
    # Q: What makes Walker2d harder? (Hint: think about balance)
    # A: TODO
    #
    # Q: What makes Ant harder? (Hint: think about coordination)
    # A: TODO
    #
    # Q: Could you now train Humanoid-v5? What would you change?
    # A: TODO

    print("  (Uncomment the code above and run each robot)")
    print("  Congratulations — you can now train any MuJoCo robot with PPO!")


if __name__ == "__main__":
    exercise_5()
