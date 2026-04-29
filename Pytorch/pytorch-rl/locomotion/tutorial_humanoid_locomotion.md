# Humanoid Locomotion Tutorial

This is the learning path for building a humanoid locomotion policy from the
basic PyTorch RL components already used in this repo.

## 1. Pick the first simulator

Use one simulator as the training source first:

- `MuJoCo`: easiest local starting point for XML/MJCF humanoid experiments.
- `Newton`: good for differentiable or research-oriented physics experiments.
- `Isaac Sim` or `Isaac Lab`: best when you want many parallel environments on
  GPU, but it has the heaviest setup.

Do not start with all three. Train in one, then transfer the same policy to the
others for sim2sim validation.

## 2. Add robot assets

Expected asset locations:

- `assets/robots/humanoid/urdf/`: your `.urdf` files.
- `assets/robots/humanoid/mjcf/`: your MuJoCo `.xml` files.
- `assets/robots/humanoid/meshes/`: `.stl`, `.obj`, or `.dae` mesh files.

If your source robot is URDF and you train in MuJoCo, convert or hand-author an
MJCF file and keep both versions here.

## 3. Define a common environment contract

Every simulator wrapper should expose the same conceptual API:

- `reset(seed=None) -> obs`
- `step(action) -> obs, reward, terminated, truncated, info`
- `observation_space`
- `action_space`

The policy should not know which simulator generated the observation. That is
what makes sim2sim possible.

## 4. Decide the observation vector

A practical first observation for humanoid walking:

- base height
- base orientation, preferably projected gravity instead of raw quaternion
- base linear velocity
- base angular velocity
- joint positions
- joint velocities
- previous action
- command velocity, such as desired forward/lateral/yaw velocity

Keep the order identical across simulators.

## 5. Decide the action vector

For a first version, use target joint positions or target joint offsets. The
simulator wrapper should convert those targets into torques through PD control.

This keeps the PPO policy easier to train than direct torque control.

## 6. Build the reward

Start simple:

- positive reward for matching commanded forward velocity
- small penalty for lateral drift
- small penalty for yaw error unless yaw is commanded
- penalty for action size
- penalty for action rate
- penalty for joint velocity
- termination penalty for falling

Only add more terms when a failure mode is clear.

## 7. Train with PPO

Use the PPO pieces from the parent repo:

- actor network outputs a Gaussian action distribution
- critic network predicts state value
- rollout buffer stores observations, actions, rewards, dones, log-probs, values
- GAE computes advantages
- PPO clipped objective updates actor and critic

For locomotion, train on batches of short rollouts across many environments if
your simulator supports vectorization.

## 8. Export the policy

Export only the policy inference path:

- observation normalization statistics
- actor weights
- action scaling values
- joint order metadata

The export should be simulator-independent.

## 9. Validate sim2sim

Run the same exported policy in another simulator and compare:

- episode return
- fall rate
- commanded velocity tracking error
- base height statistics
- joint limit violations
- action saturation

If transfer fails, first check units, joint order, axis conventions, actuator
limits, contact parameters, and observation normalization.

## Suggested implementation order

1. Fill `configs/humanoid_locomotion.yaml`.
2. Implement one file in `envs/`, usually `mujoco_humanoid_env.py` first.
3. Implement the actor/critic in `policies/`.
4. Implement rollout collection and PPO update in `train_ppo.py`.
5. Save checkpoints in `checkpoints/`.
6. Export inference artifacts to `exports/`.
7. Add the second simulator wrapper and run `sim2sim_validate.py`.

