import argparse
import os
from distutils.util import strtobool
import time
from torch.utils.tensorboard import SummaryWriter
import random
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.distributions.categorical import Categorical
import gymnasium as gym


def make_env(gym_id, seed, idx, capture_video, run_name):
    def thunk():
        env = gym.make(gym_id, render_mode="rgb_array" if capture_video else None)
        env = gym.wrappers.RecordEpisodeStatistics(env)
        if capture_video:
            if idx == 0:
                env = gym.wrappers.RecordVideo(env, f"videos/{run_name}", episode_trigger=lambda t: t % 1000 == 0)
        env.action_space.seed(seed)
        env.observation_space.seed(seed)
        return env
    return thunk

def layer_init(layer, std=np.sqrt(2), bias_const=0.0):
    nn.init.orthogonal_(layer.weight, std)
    nn.init.constant_(layer.bias, bias_const)
    return layer

class Agent(nn.Module):
    def __init__(self):
        super().__init__()

        self.critic = nn.Sequential(
            layer_init(nn.Linear(np.array(envs.single_observation_space.shape).prod(), 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 1), std=1.0)
        )
        self.actor = nn.Sequential(
            layer_init(nn.Linear(np.array(envs.single_observation_space.shape).prod(), 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, 64)),
            nn.Tanh(),
            layer_init(nn.Linear(64, envs.single_action_space.n), std=0.01)
        )

    def values(self, x):
        return self.critic(x)
    
    def get_action_and_value(self, x, action=None):
        logits = self.actor(x)
        probs = Categorical(logits=logits)
        if action is None:
            action = probs.sample()
        return action, probs.log_prob(action), probs.entropy(), self.critic(x)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exp_name', type=str, default=os.path.basename(os.getcwd()), help='the name of the environment')
    parser.add_argument('--gym-id', type=str, default='CartPole-v1', help='the id of the gym environment')
    parser.add_argument('--learning_rate', type=float, default=2.5e-4, help='the learning rate of the optimizer')
    parser.add_argument('--seed', type=int, default=0, help='the random seeds')
    parser.add_argument('--torch_deterministic', type=lambda x: bool(strtobool(x)), default=True, help='if toggled, `torch.backends.cudnn.deterministic=False`')
    parser.add_argument('--cuda', type=lambda x: bool(strtobool(x)), default=True, help='if toggled, cuda will be enabled by default')
    parser.add_argument('--track', action='store_true', default=False, help='if toggled, this experiment will be tracked with Weights and Biases')
    parser.add_argument('--wandb_project', type=str, default='cleanRL', help='the wandb project name')
    parser.add_argument('--wandb_entity', type=str, default=None, help='the entity (team) of wandb')
    parser.add_argument('--capture_video', action='store_true', default=False, help='whether to capture videos of the agent performances (check out `videos` folder)')

    # Algorithm specific arguments
    parser.add_argument('--num_envs', type=int, default=4, help='number of parallel environments')
    parser.add_argument('--num_steps', type=int, default=128, help='number of steps to run in each environment per policy rollout')
    parser.add_argument('--total_timesteps', type=int, default=500000, help='total timesteps of the experiment')
    parser.add_argument('--anneal_lr', type=lambda x: bool(strtobool(x)), default=True, help='Toggle learning rate annealing for policy and value networks')
    parser.add_argument('--gae', type=lambda x: bool(strtobool(x)), default=True, help='the lambda for the general advantage estimation')
    parser.add_argument('--gamma', type=float, default=0.99, help='the discount factor gamma')
    parser.add_argument('--gae_lambda', type=float, default=0.95, help='the lambda for the general advantage estimation')
    parser.add_argument('--num_minibatches', type=int, default=4, help='the number of minibatches')
    parser.add_argument('--update_epochs', type=int, default=4, help='the K epochs to update the policy')
    parser.add_argument('--norm_adv', type=lambda x: bool(strtobool(x)), default=True, help='Toggles advantages normalization')
    parser.add_argument('--clip_coef', type=float, default=0.2, help='the surrogate clipping coefficient')
    parser.add_argument('--clip_vloss', type=lambda x: bool(strtobool(x)), default=True, help='Toggles whether or not to use a clipped loss for the value function')
    parser.add_argument('--ent_coef', type=float, default=0.01, help='coefficient of the entropy')
    parser.add_argument('--vf_coef', type=float, default=0.5, help='coefficient of the value function')
    parser.add_argument('--max_grad_norm', type=float, default=0.5, help='the maximum norm for the gradient clipping')
    parser.add_argument('--target_kl', type=float, default=None, help='the target KL divergence threshold')

    args = parser.parse_args()
    args.batch_size = int(args.num_envs * args.num_steps)
    args.minibatch_size = args.batch_size // args.num_minibatches
    return args

if __name__ == '__main__':
    args = parse_args()
    print(args)

    run_name = f"{args.gym_id}__{args.exp_name}__{args.seed}__{int(time.time())}"
    if args.track:
        import wandb

        wandb.init(
            project=args.wandb_project,
            entity=args.wandb_entity,
            sync_tensorboard=True,
            config=vars(args), 
            name=run_name, 
            save_code=True,
        )

    writter = SummaryWriter(f"runs/{run_name}")

    writter.add_text('hyperparameters', '|param|value|\n|-|-|\n' + '\n'.join([f'|{key}|{value}|' for key, value in vars(args).items()]))

    # Seeding
    random.seed(args.seed)
    np.random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.backends.cudnn.deterministic = args.torch_deterministic
    device = torch.device('mps')

    envs = gym.vector.SyncVectorEnv(
        [make_env(args.gym_id, args.seed + i, i, args.capture_video, run_name) 
    for i in range(args.num_envs)])
    assert isinstance(envs.single_action_space, gym.spaces.Discrete), "only discrete action spaces are supported"
    print("envs.single_observation_space.shape: ", envs.single_observation_space.shape)
    print("envs.single_action_space.n: ", envs.single_action_space.n)

    agent = Agent().to(device)
    print(agent)

    optimizer = optim.Adam(agent.parameters(), lr=args.learning_rate, eps=1e-5)

    # Setup storage for n steps of data
    obs = torch.zeros((args.num_steps, args.num_envs) + envs.single_observation_space.shape).to(device)
    actions = torch.zeros((args.num_steps, args.num_envs) + envs.single_action_space.shape).to(device)
    logprobs = torch.zeros((args.num_steps, args.num_envs)).to(device)
    rewards = torch.zeros((args.num_steps, args.num_envs)).to(device)
    dones = torch.zeros((args.num_steps, args.num_envs)).to(device)
    values = torch.zeros((args.num_steps, args.num_envs)).to(device)

    global_step = 0
    s_time = time.time()
    next_obs, _ = envs.reset()
    next_obs = torch.Tensor(next_obs).to(device)
    next_done = torch.zeros(args.num_envs).to(device)
    num_updates = args.total_timesteps // args.batch_size

    print(num_updates)
    print("next_obs.shape: ", next_obs.shape)
    print("agent.get_values(next_obs): ", agent.values(next_obs))
    print("agent.get_values(next_obs).shape: ", agent.values(next_obs).shape)
    print()
    print("agent.get_action_and_value(next_obs): ", agent.get_action_and_value(next_obs))

    for update in range(1, num_updates + 1):
        if args.anneal_lr:
            lr_now = args.learning_rate * (1 - update / num_updates)
            optimizer.param_groups[0]['lr'] = lr_now

        for step in range(0, args.num_steps):
            global_step += 1 * args.num_envs
            obs[step] = next_obs
            dones[step] = next_done

            # ALGO LOGIC: action logic
            with torch.no_grad():
                action, logprob, _, value = agent.get_action_and_value(next_obs)
                values[step] = value.flatten()
            actions[step] = action
            logprobs[step] = logprob

            next_obs, reward, terminated, truncated, info = envs.step(action.cpu().numpy())
            done = terminated | truncated
            rewards[step] = torch.tensor(reward, dtype=torch.float32).to(device).view(-1)
            next_obs, next_done = torch.Tensor(next_obs).to(device), torch.Tensor(done).to(device)

            for item in info.get("final_info", []):
                if item is not None and "episode" in item:
                    print(f"global_step={global_step}, episodic_return={item['episode']['r']}")
                    writter.add_scalar("charts/episodic_return", item["episode"]["r"], global_step)
                    writter.add_scalar("charts/episodic_length", item["episode"]["l"], global_step)
                    break

        # bootstrap reward if not done
        with torch.no_grad():
            next_value = agent.values(next_obs).reshape(1, -1)
            if args.gae:
                advantages = torch.zeros_like(rewards).to(device)
                lastgaelam = 0
                for t in reversed(range(args.num_steps)):
                    if t == args.num_steps - 1:
                        nextnonterminal = 1.0 - next_done
                        nextvalues = next_value
                    else:
                        nextnonterminal = 1.0 - dones[t + 1]
                        nextvalues = values[t + 1]
                    delta = rewards[t] + args.gamma * nextvalues * nextnonterminal - values[t]
                    advantages[t] = lastgaelam = delta + args.gamma * args.gae_lambda * nextnonterminal * lastgaelam
                returns = advantages + values
            else:
                returns = torch.zeros_like(rewards).to(device)
                for t in reversed(range(args.num_steps)):
                    if t == args.num_steps - 1:
                        nextnonterminal = 1.0 - next_done
                        next_return = next_value
                    else:
                        nextnonterminal = 1.0 - dones[t + 1]
                        next_return = returns[t + 1]
                    returns[t] = rewards[t] + args.gamma * nextnonterminal * next_return
                advantages = returns - values

        # flatten the batch
        b_obs = obs.reshape((-1,) + envs.single_observation_space.shape)
        b_logprobs = logprobs.reshape(-1)
        b_actions = actions.reshape((-1,) + envs.single_action_space.shape)
        b_advantages = advantages.reshape(-1)
        b_returns = returns.reshape(-1)
        b_values = values.reshape(-1)

        # Optimizing the policy and value network
        b_inds = np.arange(args.batch_size)
        clipfracs = []
        for epoch in range(args.update_epochs):
            np.random.shuffle(b_inds)
            for start in range(0, args.batch_size, args.minibatch_size):
                end = start + args.minibatch_size
                mb_inds = b_inds[start:end]

                _, newlogprob, entropy, new_values = agent.get_action_and_value(
                    b_obs[mb_inds], b_actions.long()[mb_inds]
                )
                logratio = newlogprob - b_logprobs[mb_inds]
                ratio = logratio.exp()

                mb_advantages = b_advantages[mb_inds]
                if args.norm_adv:
                    mb_advantages = (mb_advantages - mb_advantages.mean()) / (mb_advantages.std() + 1e-8)

                # Policy loss
                pg_loss1 = -mb_advantages * ratio
                pg_loss2 = -mb_advantages * torch.clamp(ratio, 1 - args.clip_coef, 1 + args.clip_coef)
                pg_loss = torch.max(pg_loss1, pg_loss2).mean()

                # Value loss
                new_values = new_values.view(-1)
                if args.clip_vloss:
                    v_loss_unclipped = (new_values - b_returns[mb_inds]) ** 2
                    v_clipped = b_values[mb_inds] + torch.clamp(
                        new_values - b_values[mb_inds],
                        -args.clip_coef,
                        args.clip_coef,
                    )
                    v_loss_clipped = (v_clipped - b_returns[mb_inds]) ** 2
                    v_loss_max = torch.max(v_loss_unclipped, v_loss_clipped)
                    v_loss = 0.5 * v_loss_max.mean()
                else:
                    v_loss = 0.5 * ((new_values - b_returns[mb_inds]) ** 2).mean()

                with torch.no_grad():
                    old_approx_kl = (-logratio).mean()
                    approx_kl = ((ratio - 1) - logratio).mean()
                    clipfracs += [((ratio - 1.0).abs() > args.clip_coef).float().mean().item()]

                entropy_loss = entropy.mean()
                loss = pg_loss - args.ent_coef * entropy_loss + v_loss * args.vf_coef

                optimizer.zero_grad()
                loss.backward()
                nn.utils.clip_grad_norm_(agent.parameters(), args.max_grad_norm)
                optimizer.step()

            if args.target_kl is not None:
                if approx_kl > args.target_kl:
                    break

        y_pred, y_true = b_values.cpu().numpy(), b_returns.cpu().numpy()
        var_y = np.var(y_true)
        explained_var = np.nan if var_y == 0 else 1 - np.var(y_true - y_pred) / var_y

        # TRY NOT TO MODIFY: record rewards for plotting purposes
        writter.add_scalar("charts/learning_rate", optimizer.param_groups[0]["lr"], global_step)
        writter.add_scalar("losses/value_loss", v_loss.item(), global_step)
        writter.add_scalar("losses/policy_loss", pg_loss.item(), global_step)
        writter.add_scalar("losses/entropy", entropy_loss.item(), global_step)
        writter.add_scalar("losses/approx_kl", approx_kl.item(), global_step)
        writter.add_scalar("losses/clipfrac", np.mean(clipfracs), global_step)
        writter.add_scalar("losses/explained_variance", explained_var, global_step)
        print("SPS:", int(global_step / (time.time() - s_time)))
        writter.add_scalar("charts/SPS", int(global_step / (time.time() - s_time)), global_step)

    envs.close()
    writter.close()
