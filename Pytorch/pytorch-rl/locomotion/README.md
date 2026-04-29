# Humanoid Locomotion

This folder is a small scaffold for learning humanoid locomotion with the RL
building blocks from the parent repo: policy networks, value networks, GAE, and
PPO.

The intended flow is:

1. Put your robot assets in `assets/robots/humanoid/`.
2. Fill one simulator environment in `envs/`.
3. Train with PPO in the source simulator.
4. Export the policy.
5. Run `sim2sim_validate.py` against another simulator to check transfer.

Start with `tutorial_humanoid_locomotion.md`, then use
`humanoid_locomotion_boilerplate.py` as the minimal scratch file.

