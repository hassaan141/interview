from envs.base_humanoid_env import BaseHumanoidEnv


class NewtonHumanoidEnv(BaseHumanoidEnv):
    """Newton implementation placeholder for sim2sim validation."""

    def __init__(self, config):
        self.config = config
        # Import Newton here.
        # Load config["assets"]["urdf"] or the Newton-native robot description.
        # Match the same observation and action contract used by MuJoCo.

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

