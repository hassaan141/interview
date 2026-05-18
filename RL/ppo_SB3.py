"""
PPO on CartPole using Stable-Baselines3
========================================
A simple reference for how to train, evaluate, inspect, and save a PPO agent.
Maps directly to the from-scratch ppo.py concepts.

Install: pip install stable-baselines3[extra] gymnasium[classic-control]
"""

import gymnasium as gym
import numpy as np
import torch
from stable_baselines3 import PPO
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.evaluation import evaluate_policy
from stable_baselines3.common.callbacks import BaseCallback


# =============================================================================
# 1) Train PPO on CartPole — the minimal version
# =============================================================================

env = make_vec_env("CartPole-v1", n_envs=4, seed=42)

model = PPO(
    "MlpPolicy",               # Feedforward network (not CNN)
    env,

    # --- Rollout ---
    n_steps=128,                # Steps per env before each update (num_steps in our code)
    n_epochs=4,                 # Passes over collected data (update_epochs)
    batch_size=128,             # Minibatch size for gradient updates

    # --- PPO core ---
    clip_range=0.2,             # Epsilon for ratio clipping — the key PPO trick
    clip_range_vf=None,         # Value clipping (None = off, often works better)

    # --- Loss weights ---
    ent_coef=0.01,              # Entropy bonus — encourages exploration
    vf_coef=0.5,                # Value loss weight

    # --- GAE ---
    gamma=0.99,                 # Discount factor
    gae_lambda=0.95,            # GAE smoothing parameter
    normalize_advantage=True,   # Normalize advantages per minibatch

    # --- Optimization ---
    learning_rate=2.5e-4,       # Adam LR
    max_grad_norm=0.5,          # Gradient clipping

    # --- Network architecture (same as our ppo.py) ---
    policy_kwargs=dict(
        net_arch=dict(pi=[64, 64], vf=[64, 64]),
        activation_fn=torch.nn.Tanh,
    ),

    verbose=1,
    seed=42,
    tensorboard_log="./ppo_cartpole_logs/",
)

print("\n--- Training ---")
model.learn(total_timesteps=50_000)


# =============================================================================
# 2) Evaluate the trained agent
# =============================================================================

print("\n--- Evaluation ---")
mean_reward, std_reward = evaluate_policy(model, model.get_env(), n_eval_episodes=20)
print(f"Trained PPO:  {mean_reward:.1f} +/- {std_reward:.1f}")


# =============================================================================
# 3) Watch the agent play one episode
# =============================================================================

print("\n--- Single episode rollout ---")
eval_env = gym.make("CartPole-v1")
obs, _ = eval_env.reset(seed=123)

total_reward = 0
step = 0
done = False

while not done:
    # model.predict() replaces env.action_space.sample()
    action, _ = model.predict(obs, deterministic=True)
    obs, reward, terminated, truncated, _ = eval_env.step(action)
    total_reward += reward
    step += 1
    done = terminated or truncated

    if step <= 5 or step % 100 == 0:
        print(f"  step {step:3d} | action={action} | cart={obs[0]:+.3f} pole_angle={obs[2]:+.4f}")

print(f"  Episode finished: {step} steps, reward={total_reward:.0f}")
eval_env.close()


# =============================================================================
# 4) Peek inside the networks
# =============================================================================

print("\n--- Network architecture ---")
print(f"Actor:  {model.policy.mlp_extractor.policy_net} → {model.policy.action_net}")
print(f"Critic: {model.policy.mlp_extractor.value_net} → {model.policy.value_net}")

# Query directly, just like our theory diagrams
test_obs = torch.tensor([[0.0, 0.0, 0.05, 0.0]])  # slightly tilted pole
with torch.no_grad():
    dist = model.policy.get_distribution(test_obs)
    probs = dist.distribution.probs[0]
    value = model.policy.predict_values(test_obs)
    print(f"\nFor a slightly right-tilted pole:")
    print(f"  P(left)={probs[0]:.3f}  P(right)={probs[1]:.3f}")
    print(f"  V(s)={value.item():.2f}")


# =============================================================================
# 5) Save and load
# =============================================================================

model.save("ppo_cartpole")
print("\n--- Saved to ppo_cartpole.zip ---")

loaded = PPO.load("ppo_cartpole")
mean_r, _ = evaluate_policy(loaded, gym.make("CartPole-v1"), n_eval_episodes=10)
print(f"Loaded model check: {mean_r:.1f} avg reward")