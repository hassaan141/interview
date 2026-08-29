import random
from pathlib import Path

import ale_py
import gymnasium as gym
import numpy as np
import torch.nn as nn
import torch.nn.functional as F
import torch
import matplotlib.pyplot as plt
from stable_baselines3.common.atari_wrappers import MaxAndSkipEnv
from torch.utils.data import DataLoader, TensorDataset

gym.register_envs(ale_py)

ARTIFACT_DIR = Path(__file__).resolve().parent / "checkpoints" / "ppo_100lines"

class ActorCritic(nn.Module):
    def __init__(self, nb_actions):
        super().__init__()

        self.head = nn.Sequential(
            nn.Conv2d(4, 16, 8, stride=4), nn.Tanh(), # convuluting the network input layers 
            nn.Conv2d(16, 32, 4, stride=2), nn.Tanh(),
            nn.Flatten(), nn.Linear(2592, 256), nn.Tanh(),
        )

        self.actor = nn.Sequential(nn.Linear(256, nb_actions)) # returns number of actions 
        self.critic = nn.Sequential(nn.Linear(256, 1)) # returns how good the action we took was
    
    def forward(self, x):
        h = self.head(x)
        return self.actor(h), self.critic(h)
    
class Environments():

    def __init__(self, nb_actor):

        self.envs = [self.get_env() for _ in range(nb_actor)] # keeping all the buffers empty for now
        self.observations = [None for _ in range(nb_actor)]
        self.current_life = [None for _ in range(nb_actor)]
        self.done = [False for _ in range(nb_actor)]
        self.total_rewards = [0 for _ in range(nb_actor)]
        self.nb_actor = nb_actor

        for env_id in range(nb_actor):
            self.reset_env(env_id) # reset env when initializing

        
    def len(self):
        return self.nb_actor # number of parallel evs

    def reset_env(self, env_id):
        self.total_rewards[env_id] = 0 # reseting reward
        self.envs[env_id].reset()

        for _ in range(self.nb_actor): # this if for noops, after reset, we give some steps so each one of our nb_envs is in a different state
            self.observations[env_id], reward, terminated, truncated, info = self.envs[env_id].step(1)
            self.total_rewards[env_id] += reward
            self.current_life[env_id] = info['lives']

    
    def step(self, env_id, action):
        next_obs, reward, terminated, truncated, info = self.envs[env_id].step(int(action.item())) # steping to next state depending on our action
        dead = terminated or truncated
        done = True if (info['lives'] < self.current_life[env_id]) else False # if we lose a life, then done is True (can get the batch even if done is False)
        self.done[env_id] = done
        self.total_rewards[env_id] += reward # accumulate reward
        self.current_life[env_id] = info['lives'] # current life
        self.observations[env_id] = next_obs # what the next state is 
        return next_obs, reward, dead, done, info
    
    def get_env(self):

        env = gym.make("BreakoutNoFrameskip-v4") # our env
        env = gym.wrappers.RecordEpisodeStatistics(env) # This is a wrapper which saves episode information
        env = gym.wrappers.ResizeObservation(env, (84, 84)) # resize sapce from 256x128 -> 84x84 keeping important features
        env = gym.wrappers.GrayscaleObservation(env) # change from color to grayscale
        env = gym.wrappers.FrameStackObservation(env, 4) # dont know waht this sdoes
        env = MaxAndSkipEnv(env, skip=4) # take teh max of 4 frames, our algorothim doesnt need to run at 60 fps
        return env
    

def PPO(envs, T=128, K=3, batch_size=32*8, gamma=0.98, device='mps', gae_parameter=0.95, vf_coeff_c1=1, ent_coef_c2=0.01, nb_iterations=40_000 ): # have all of these inside as global variables

    ARTIFACT_DIR.mkdir(parents=True, exist_ok=True)
    optimizer = torch.optim.Adam(actorcritic.parameters(), lr=2.5e-4)
    scheduler = torch.optim.lr_scheduler.LinearLR( # understand what this is
        optimizer, start_factor=1., end_factor=0.0, total_iters=nb_iterations
    )

    max_reward = 0
    total_rewards = [[] for _ in range(envs.len())]
    smoothed_rewards = [[] for _ in range(envs.len())]

    for iteration in range(nb_iterations):
        advantages = torch.zeros((envs.len(), T), dtype=torch.float32, device=device)
        buffer_states = torch.zeros((envs.len(), T, 4, 84, 84), dtype=torch.float32, device=device)
        buffer_actions = torch.zeros((envs.len(), T), dtype=torch.float32, device=device)
        buffer_logprobs = torch.zeros((envs.len(), T), dtype=torch.float32, device=device)
        buffer_state_values = torch.zeros((envs.len(), T+1), dtype=torch.float32, device=device)
        buffer_rewards = torch.zeros((envs.len(), T), dtype=torch.float32, device=device)
        buffer_is_terminal = torch.zeros((envs.len(), T), dtype=torch.float32, device=device)

        for env_id in range(envs.len()): # Run for each env, in our case 8
            with torch.no_grad():

                for t in range(T): # run policy old in environement for T timesteps

                    obs = torch.from_numpy(envs.observations[env_id] / 255.).unsqueeze(0).float().to(device)
                    action_logits, value = actorcritic(obs)
                    action_logits, value = action_logits.squeeze(0), value.squeeze(0) # getting logits because we need both logits and probs, if we use softmax, getting log probs might be hard
                    m = torch.distributions.categorical.Categorical(logits=action_logits) # fetch the probabilites using this, directly compute the log probabilies in an efficent way

                    if envs.done[env_id]: # Fire to reset env
                        action = torch.tensor([1]).to(device) # if we are done, we need to reset the env, which is 1
                    else:
                        action = m.sample() # if not done sample normally

                    log_prob = m.log_prob(action)
                    _, reward, dead, done, _  = envs.step(env_id, action)
                    reward = np.sign(reward) # only care about the sign, not the magnitude

                    buffer_states[env_id, t] = obs
                    buffer_actions[env_id, t] = torch.tensor([action]).to(device)
                    buffer_logprobs[env_id, t] = log_prob
                    buffer_state_values[env_id, t] = value
                    buffer_rewards[env_id, t] = reward
                    buffer_is_terminal[env_id, t] = done

                    if dead:
                        if envs.total_rewards[env_id] > max_reward:
                            max_reward = envs.total_rewards[env_id]
                            torch.save(actorcritic.cpu(), ARTIFACT_DIR / f"actorcritic_{max_reward}")
                            actorcritic.to(device)

                        total_rewards[env_id].append(envs.total_rewards[env_id])
                        envs.reset_env(env_id)
                
                buffer_state_values[env_id, T] = actorcritic(
                    torch.from_numpy(envs.observations[env_id] / 255.).unsqueeze(0).float().to(device))[1].squeeze(0
                    ) # we need the value of the last state to compute the advantage for the last step

                # Compute advantage estimates A^1; ... ; A^T
                for t in range(T-1, -1, -1):
                    next_non_terminal = 1.0 - buffer_is_terminal[env_id, t]
                    delta_t = buffer_rewards[env_id, t] + gamma * buffer_state_values[
                        env_id, t+1] * next_non_terminal - buffer_state_values[env_id, t]
                    if t == (T-1):
                        A_t = delta_t
                    else:
                        A_t = delta_t + gamma * gae_parameter * advantages[env_id, t+1] * next_non_terminal
                    advantages[env_id, t] = A_t
        
        if (iteration % 400 == 0) and iteration > 0:
            for env_id in range(envs.len()):
                smoothed_rewards[env_id].append(np.mean(total_rewards[env_id]))
                plt.plot(smoothed_rewards[env_id])
            total_rewards = [[] for _ in range(envs.len())]
            plt.title("Average Reward on Breakout")
            plt.xlabel("Training Epochs")
            plt.ylabel("Average Reward per Episode")
            plt.savefig(ARTIFACT_DIR / 'average_reward_on_breakout.png')
            plt.close()
        
        for epoch in range(K):
            advantages_data_loader = DataLoader(
                TensorDataset(advantages.reshape(advantages.shape[0] * advantages.shape[1]),
                                buffer_states.reshape(-1, buffer_states.shape[2], buffer_states.shape[3],
                                                        buffer_states.shape[4]),
                                buffer_actions.reshape(-1),
                                buffer_logprobs.reshape(-1),
                                buffer_state_values[:, :T].reshape(-1),),
                batch_size=batch_size, shuffle=True,)

            for batch_advantages in advantages_data_loader:
                b_adv, obs, action_that_was_taken, old_log_prob, old_state_values = batch_advantages

                logits, value = actorcritic(obs)
                logits, value = logits.squeeze(0), value.squeeze(-1)
                m = torch.distributions.categorical.Categorical(logits=logits)
                log_prob = m.log_prob(action_that_was_taken)
                ratio = torch.exp(log_prob - old_log_prob)
                returns = b_adv + old_state_values

                # clipped surrogate objective
                policy_loss_1 = b_adv * ratio
                alpha = 1. - iteration / nb_iterations
                clip_range = 0.1 * alpha
                policy_loss_2 = b_adv * torch.clamp(ratio, 1 - clip_range, 1 + clip_range)
                policy_loss = -torch.min(policy_loss_1, policy_loss_2).mean()

                value_loss1 = F.mse_loss(returns, value, reduction='none')
                value_loss2 = F.mse_loss(returns, torch.clamp(value, value - clip_range, value + clip_range),
                                         reduction='none')
                value_loss = torch.max(value_loss1, value_loss2).mean()

                loss = policy_loss + ent_coef_c2 * -(m.entropy()).mean() + vf_coeff_c1 * value_loss

                # θ_old <- θ
                optimizer.zero_grad()
                loss.backward()
                torch.nn.utils.clip_grad_norm_(actorcritic.parameters(), 0.5)
                optimizer.step()

        scheduler.step()


if __name__ == '__main__':
    device = 'mps' if torch.backends.mps.is_available() else 'cpu'
    nb_actor = 8
    envs = Environments(nb_actor)
    actorcritic = ActorCritic(envs.envs[0].action_space.n).to(device)
    PPO(envs, device=device)
