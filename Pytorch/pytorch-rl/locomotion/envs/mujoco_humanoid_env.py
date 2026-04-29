from envs.base_humanoid_env import BaseHumanoidEnv


class MujocoHumanoidEnv(BaseHumanoidEnv):
    """MuJoCo implementation placeholder.

    Fill this first if you want the fastest local path. Keep observation order,
    action scaling, and reward terms identical to the other simulator wrappers.
    """

    def __init__(self, config):
        self.config = config
        # Import mujoco or gymnasium MuJoCo bindings here.
        # Load config["assets"]["mjcf"].
        # Build observation_space and action_space.

    def reset(self, seed=None):
        # Reset MuJoCo state and return the first observation.
        raise NotImplementedError

    def step(self, action):
        # Convert action to PD targets, step physics, compute reward, return Gymnasium tuple.
        raise NotImplementedError

    @property
    def observation_space(self):
        raise NotImplementedError

    @property
    def action_space(self):
        raise NotImplementedError

