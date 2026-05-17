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
        env = gym.make(gym_id)
        env = gym.wrappers.RecordEpisodeStatistics(env)
        if capture_video:
            if idx == 0:
                env = gym.wrappers.RecordVideo(env, f"videos/{run_name}", record_video_trigger=lambda t: t % 1000 == 0)
        env.seed(seed)
        env.action_space.seed(seed)
        env.observation_space.seed(seed)
        return env
    return thunk

class Agent(nn.Module):
    def __init__(self):
        super().__init__()



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
    parser.add_argument('--capture_video', type=lambda x: bool(strtobool(x)), default=False, help='whether to capture videos of the agent performances (check out `videos` folder)')
    args = parser.parse_args()
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
            monitor_gym=True,
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

    
