"""
runner_env.py — A custom Gymnasium environment for the simple biped.

This is the Python side of what gym.make("Walker2d-v5") hides from you.
Every gym.make() call ultimately creates a class just like this one.

The three things YOU must define:
  1. _get_obs()    — what the agent sees each step
  2. step()        — physics advance + reward + termination
  3. reset_model() — starting state at episode begin

Everything else (rendering, VecEnv compatibility, seeding) is handled
by the MujocoEnv base class we inherit from.
"""

from __future__ import annotations

from pathlib import Path

import numpy as np
import gymnasium as gym
from gymnasium import spaces
from gymnasium.envs.mujoco import MujocoEnv

# Absolute path to the XML — works regardless of where you run the script.
DEFAULT_XML = str(Path(__file__).parent / "simple_biped.xml")


class SimpleBipedEnv(MujocoEnv):
    """
    Custom MuJoCo environment: a 2D bipedal runner.

    Observation space (17 values):
    ┌─────────────────────────────────────────────────────────────────┐
    │  qpos[1]        torso z (absolute height)                       │
    │  qpos[2]        torso tilt (radians, + = leaning forward)       │
    │  qpos[3..8]     6 joint angles in radians                       │
    │  qvel[0]        forward speed (THIS is what we want to max)     │
    │  qvel[1]        vertical speed                                  │
    │  qvel[2]        tilt rate                                       │
    │  qvel[3..8]     6 joint angular velocities                      │
    └─────────────────────────────────────────────────────────────────┘

    Action space (6 values in [-1, 1]):
        right_hip, right_knee, right_ankle, left_hip, left_knee, left_ankle
    """

    metadata = {"render_modes": ["human", "rgb_array", "depth_array"]}

    def __init__(
        self,
        xml_file: str = DEFAULT_XML,
        frame_skip: int = 4,
        # ── reward knobs ──────────────────────────────────────────────
        forward_reward_weight: float = 2.5,   # higher = agent cares more about speed
        ctrl_cost_weight: float = 0.001,       # penalty for large joint torques
        healthy_reward: float = 1.0,           # reward per step for staying upright
        target_speed: float | None = 3.0,      # aim for this m/s; None = no target
        # ── termination ───────────────────────────────────────────────
        terminate_when_unhealthy: bool = True,
        healthy_z_range: tuple[float, float] = (0.7, 2.0),    # torso height
        healthy_angle_range: tuple[float, float] = (-1.0, 1.0), # torso tilt radians
        # ── misc ──────────────────────────────────────────────────────
        reset_noise_scale: float = 0.005,
        render_mode: str | None = None,
    ):
        self._forward_reward_weight = forward_reward_weight
        self._ctrl_cost_weight = ctrl_cost_weight
        self._healthy_reward = healthy_reward
        self._target_speed = target_speed
        self._terminate_when_unhealthy = terminate_when_unhealthy
        self._healthy_z_range = healthy_z_range
        self._healthy_angle_range = healthy_angle_range
        self._reset_noise_scale = reset_noise_scale

        # Tell gymnasium what shapes to expect. Must match _get_obs().
        observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(17,), dtype=np.float64
        )

        # MujocoEnv.__init__ loads the XML, creates model + data,
        # sets up the renderer, and defines self.action_space.
        super().__init__(
            model_path=xml_file,
            frame_skip=frame_skip,
            observation_space=observation_space,
            render_mode=render_mode,
        )

    # ──────────────────────────────────────────────────────────────────
    # 1. OBSERVATION
    # ──────────────────────────────────────────────────────────────────

    def _get_obs(self) -> np.ndarray:
        """
        Build the observation vector from MuJoCo's internal state.

        self.data.qpos — joint positions (9 values for our robot)
          [0] rootx  — absolute x position (skip: agent doesn't need to know WHERE it is)
          [1] rootz  — absolute z (height)
          [2] rooty  — tilt angle
          [3..8]     — 6 joint angles

        self.data.qvel — joint velocities (9 values)
          [0] forward speed  ← the agent NEEDS this to know how fast it's going
          [1] vertical speed
          [2] tilt rate
          [3..8] joint angular velocities
        """
        # Skip qpos[0] (rootx) — absolute x position is not informative for gait.
        # The agent should behave the same whether it's at x=0 or x=1000.
        position = self.data.qpos[1:].copy()   # 8 values
        velocity = self.data.qvel[:].copy()    # 9 values
        return np.concatenate([position, velocity])  # 17 total

    # ──────────────────────────────────────────────────────────────────
    # 2. STEP (advance physics + compute reward)
    # ──────────────────────────────────────────────────────────────────

    def step(self, action: np.ndarray):
        """
        The main loop: apply action → advance physics → compute reward.

        Returns:
          obs         — new observation (17 values)
          reward      — scalar reward signal
          terminated  — True if robot fell (episode ends early)
          truncated   — True if max steps reached (set by TimeLimit wrapper)
          info        — dict of extra diagnostics
        """
        # Record x position BEFORE stepping so we can measure displacement.
        x_before = self.data.qpos[0]

        # Apply the action and run physics forward by frame_skip steps.
        # self.dt = frame_skip * timestep = 4 * 0.002 = 0.008 s per agent step.
        self.do_simulation(action, self.frame_skip)

        x_after = self.data.qpos[0]
        x_velocity = (x_after - x_before) / self.dt  # metres per second

        # ── REWARD COMPONENTS ─────────────────────────────────────────
        #
        # 1. Forward speed reward — the main thing we want
        forward_reward = self._forward_reward_weight * x_velocity

        # 2. Alive reward — small per-step bonus for staying upright.
        #    This matters early in training: without it, falling immediately
        #    is "neutral" (reward=0) instead of costly, which slows learning.
        alive_reward = self._healthy_reward if self._is_healthy else 0.0

        # 3. Control cost — penalise large torques.
        #    Without this the agent thrashes joints at max torque constantly.
        #    We use a very small weight (0.001) because running needs effort.
        ctrl_cost = self._ctrl_cost_weight * float(np.sum(np.square(action)))

        # 4. Velocity-target bonus (optional) ──────────────────────────
        #    Instead of "faster = always better", this gives a bonus that
        #    PEAKS at target_speed and falls off on either side.
        #    The shape is a downward parabola: 1 - ((v - target) / 1.0)²
        #    At v = target:           bonus = 1.0  (full reward)
        #    At v = target ± 1 m/s:   bonus = 0.0
        velocity_bonus = 0.0
        if self._target_speed is not None:
            deviation = (x_velocity - self._target_speed) / 1.0
            velocity_bonus = float(max(0.0, 1.0 - deviation ** 2))

        reward = forward_reward + alive_reward - ctrl_cost + velocity_bonus
        # ──────────────────────────────────────────────────────────────

        obs = self._get_obs()
        terminated = (not self._is_healthy) if self._terminate_when_unhealthy else False
        truncated = False  # handled by gymnasium's TimeLimit wrapper

        info = {
            "x_velocity": x_velocity,
            "forward_reward": forward_reward,
            "alive_reward": alive_reward,
            "ctrl_cost": ctrl_cost,
            "velocity_bonus": velocity_bonus,
        }

        return obs, reward, terminated, truncated, info

    # ──────────────────────────────────────────────────────────────────
    # 3. RESET
    # ──────────────────────────────────────────────────────────────────

    def reset_model(self) -> np.ndarray:
        """
        Called by gymnasium at the start of each episode.
        Sets the robot to a near-default standing pose + small random noise.

        self.init_qpos — default joint values loaded from the XML
          For slide joints:  the body's initial world position (rootx=0, rootz=1.25)
          For hinge joints:  0 radians (standing straight)

        Adding noise prevents the agent from memorising a single fixed starting
        pose — it must learn to run from slightly different initial conditions.
        """
        noise = self._reset_noise_scale
        qpos = self.init_qpos + self.np_random.uniform(-noise, noise, size=self.model.nq)
        qvel = self.init_qvel + self.np_random.uniform(-noise, noise, size=self.model.nv)
        self.set_state(qpos, qvel)
        return self._get_obs()

    # ──────────────────────────────────────────────────────────────────
    # HELPERS
    # ──────────────────────────────────────────────────────────────────

    @property
    def _is_healthy(self) -> bool:
        """
        The episode ends (terminated=True) when the robot is unhealthy.

        qpos[1] = rootz = absolute z of torso in metres.
          • Starts at 1.25 (XML pos).
          • Healthy standing: ~1.1 – 1.2.
          • Fallen: < 0.7.

        qpos[2] = rooty = torso tilt angle in radians.
          • 0 = perfectly upright.
          • |angle| > 1.0 rad (~57°) = clearly tipping over.
        """
        z = float(self.data.qpos[1])
        angle = float(self.data.qpos[2])
        z_ok = self._healthy_z_range[0] < z < self._healthy_z_range[1]
        angle_ok = self._healthy_angle_range[0] < angle < self._healthy_angle_range[1]
        return z_ok and angle_ok
