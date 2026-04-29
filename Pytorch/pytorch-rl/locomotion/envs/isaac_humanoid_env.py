from envs.base_humanoid_env import BaseHumanoidEnv


class IsaacHumanoidEnv(BaseHumanoidEnv):
    """Isaac Sim or Isaac Lab implementation placeholder for GPU validation."""

    def __init__(self, config):
        self.config = config
        # Import Isaac Sim or Isaac Lab here.
        # Load config["assets"]["urdf"] and mesh assets.
        # Keep joint names, units, observation order, and action scaling aligned.

    def reset(self, seed=None):
        raise NotImplementedError

    def step(self, action):
        raise NotImplementedError

    @property
    def observation_space(self):
        raise NotImplementedError

    @property
    def action_space(self):
        raise NotImplementedError

