# Environment Wrappers

Each file in this folder should adapt one simulator to the same humanoid
locomotion API.

Fill these in this order:

1. `mujoco_humanoid_env.py`
2. `newton_humanoid_env.py`
3. `isaac_humanoid_env.py`

The wrappers should hide simulator differences from PPO. The actor should see
the same observation shape and action semantics in every simulator.

