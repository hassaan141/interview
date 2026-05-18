"""
EXERCISE 1: Explore the environment
Goal: Understand what the robot sees and does before training anything.

Tasks:
  1a. Create a HalfCheetah-v5 environment
  1b. Print the observation space shape and action space shape
  1c. Run 1 episode with random actions, print obs/action/reward at each step
  1d. Answer: What do the observation dimensions represent?
             What range are the actions in? What does the reward look like?

Install first:
    pip install stable-baselines3[extra] gymnasium[mujoco]
"""

import gymnasium as gym


def exercise_1():
    print("=" * 60)
    print("EXERCISE 1: Explore the environment")
    print("=" * 60)

    # 1a. Create the environment
    # HINT: gym.make("HalfCheetah-v5")
    env = None  # TODO

    # 1b. Print observation and action spaces
    # HINT: env.observation_space, env.action_space
    # TODO

    # 1c. Run one episode with random actions
    # HINT: This is the same reset() → step() loop from your Lesson 0 notebook.
    #       But now actions are continuous vectors, not discrete integers.
    obs, info = None, None  # TODO: env.reset()
    total_reward = 0

    for step in range(100):  # Just 100 steps to see what's happening
        # TODO: Sample a random action with env.action_space.sample()
        action = None  # TODO

        # TODO: Take the step
        # obs, reward, terminated, truncated, info = env.step(action)

        # Print first 5 steps to see what's going on
        if step < 5:
            print(f"  Step {step}")
            print(f"    obs shape: {obs.shape}, first 5 values: {obs[:5]}")
            print(f"    action: {action}")
            # print(f"    reward: {reward:.3f}")

        # TODO: Accumulate total_reward

    print(f"\n  Total reward over 100 random steps: {total_reward:.1f}")
    print(f"  (This will be bad — random actions can't walk!)")

    # 1d. Questions to answer (write your answers as comments):
    # Q: How many numbers does the robot observe?
    # A: TODO
    #
    # Q: How many joints does it control?
    # A: TODO
    #
    # Q: What range are actions in? (check env.action_space.low/high)
    # A: TODO
    #
    # Q: Is the reward mostly positive or negative with random actions?
    # A: TODO

    env.close()


if __name__ == "__main__":
    exercise_1()
