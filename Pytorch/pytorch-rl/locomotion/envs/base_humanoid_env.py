from abc import ABC, abstractmethod


class BaseHumanoidEnv(ABC):
    """Common contract each simulator wrapper should implement."""

    @abstractmethod
    def reset(self, seed=None):
        raise NotImplementedError

    @abstractmethod
    def step(self, action):
        raise NotImplementedError

    @property
    @abstractmethod
    def observation_space(self):
        raise NotImplementedError

    @property
    @abstractmethod
    def action_space(self):
        raise NotImplementedError

    def close(self):
        pass

