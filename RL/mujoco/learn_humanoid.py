"""
What is Humanoid-v5?
Is a 3d robot environment in mujoco

What does it observe?
obseves its states

What actions can I send?
The actions it can take in the state space

What happens if I send random actions?
Go in a next state

When does the episode end?
What reward does MuJoCo give me?

"""

import gymnasium as gym

env = gym.make("Humanoid-v5", render_mode="human")

obs, info = env.reset(seed=42)

print(f"Observation shape: {obs.shape}")
print(f"Action space: {env.action_space}")

for step in range(1000):
    action = env.action_space.sample()
    obs, reward, terminated, trucated, info = env.step(action)

    if step % 50 == 0:
        print(f"Step {step:3d} | reward={reward:.3f} | terminated={terminated} | truncated={trucated}") 

    if terminated or trucated:
        obs, info = env.reset()

env.close()