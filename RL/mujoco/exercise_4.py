"""
EXERCISE 4: Inspect the trained agent
Goal: Look inside the Gaussian policy. Understand what it learned.

Tasks:
  4a. Load the trained model
  4b. Run a rollout and record joint torques at each step
  4c. Look at the Gaussian distribution parameters
  4d. Answer: Did the agent learn a rhythmic gait?

Requires: Run exercise_3.py first to generate halfcheetah_ppo_tuned.zip

Install first:
    pip install stable-baselines3[extra] gymnasium[mujoco]
"""

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO


def exercise_4():
    print("=" * 60)
    print("EXERCISE 4: Inspect the trained agent")
    print("=" * 60)

    # 4a. Load the model (or use the one from Exercise 3)
    # HINT: PPO.load("halfcheetah_ppo_tuned")
    try:
        model = PPO.load("halfcheetah_ppo_tuned")
        print("Loaded saved model")
    except FileNotFoundError:
        print("No saved model found. Run Exercise 3 first!")
        return

    # 4b. Run a rollout and record actions
    eval_env = gym.make("HalfCheetah-v5")
    obs, _ = eval_env.reset(seed=123)

    actions_log = []
    rewards_log = []
    obs_log = []
    done = False
    step = 0

    while not done and step < 500:
        action, _ = model.predict(obs, deterministic=True)
        obs, reward, terminated, truncated, _ = eval_env.step(action)
        done = terminated or truncated

        actions_log.append(action.copy())
        rewards_log.append(reward)
        obs_log.append(obs.copy())
        step += 1

    actions_log = np.array(actions_log)
    print(f"\n  Ran {step} steps, total reward: {sum(rewards_log):.1f}")
    print(f"  Actions shape: {actions_log.shape}")

    # TODO: Print mean action per joint — which joints are most active?
    # HINT: np.mean(np.abs(actions_log), axis=0)
    print("\n  Mean absolute action per joint:")
    print(np.mean(np.abs(actions_log), axis=0))
    # This tells you which joints the agent relies on most
    # TODO

    # 4c. Look at the Gaussian parameters for a specific state
    # HINT: The policy outputs mean and std for each joint
    test_obs = torch.tensor(obs_log[0], dtype=torch.float32).unsqueeze(0)
    with torch.no_grad():
        dist = model.policy.get_distribution(test_obs)
        mean = dist.distribution.mean[0]
        std = dist.distribution.stddev[0]

    print(f"\n  Gaussian policy for initial state:")
    joint_names = ["back_hip", "back_knee", "back_ankle",
                   "front_hip", "front_knee", "front_ankle"]
    for i, name in enumerate(joint_names):
        print(f"    {name:12s}: mean={mean[i]:+.3f}  std={std[i]:.3f}")

    # 4d. Questions:
    # Q: Are the standard deviations small or large? What does that tell you?
    # A: TODO (small std = confident, large = still exploring)
    #
    # Q: Plot actions_log[:, 0] and actions_log[:, 3] (back_hip and front_hip).
    #    Do they alternate? That would be a gait pattern.
    # A: TODO
    #
    # BONUS: Uncomment to plot the joint torques over time:
    import matplotlib.pyplot as plt
    fig, axes = plt.subplots(2, 3, figsize=(14, 6), sharex=True)
    for i, (ax, name) in enumerate(zip(axes.flat, joint_names)):
        ax.plot(actions_log[:, i], linewidth=0.8)
        ax.set_title(name)
        ax.set_ylim(-1.1, 1.1)
        ax.axhline(0, color="gray", linewidth=0.5)
    axes[1, 1].set_xlabel("Timestep")
    fig.suptitle("Joint Torques During Walking")
    plt.tight_layout()
    plt.savefig("joint_torques.png", dpi=150)
    plt.show()

    eval_env.close()


if __name__ == "__main__":
    exercise_4()
