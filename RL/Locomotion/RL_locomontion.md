### RL LOCOMOTION PAPER NOTES

# Training Locomotion Policies with Massive Parallelism

## Main Point

- Train quadruped locomotion in minutes, not days, by running thousands of robots in parallel on one GPU.
- Key trick: keep simulation, rewards, policy inference, and training all on the GPU.

## PPO Scaling

- PPO batch size = `n_robots x n_steps`.
- More robots means fewer steps per robot, but not too few because GAE needs reward sequences.
- Good setting: `4096` robots, about `24` steps each, around `98k` samples per update.
- Use very large mini-batches, often tens of thousands, for stable learning.

## Reset Handling

- Falling = real episode end, so future value is `0`.
- Timeout is not failure, so bootstrap using the critic's value estimate.
- This small fix improves final reward by about `10-20%`.

## Curriculum

- Terrain is like game levels: robots move to harder terrain when they succeed.
- If a robot performs badly, it moves to easier terrain.
- This works well because thousands of robots naturally spread across difficulty levels.

## Robot Policy

- Inputs: robot body/joint state, previous actions, and terrain height samples.
- Actions: target joint positions sent to a PD controller.
- No gait is hard-coded; the robot discovers trotting by itself.

## Sim-to-Real

- They randomize friction, add sensor noise, and push robots during training.
- They use a learned actuator model to better match real ANYmal motors.
- Real robot transfer works, but high-speed performance is limited by imperfect terrain sensing.

## Results

- Flat terrain training: under `4` minutes.
- Rough terrain training: under `20` minutes.
- About `100-400x` faster than earlier locomotion training pipelines.

## Limitations

- Still needs reward tuning to avoid weird movement artifacts.
- Depends heavily on NVIDIA GPU + Isaac Gym.
- Real-world robustness is not the main contribution; speed and pipeline design are.

# Learning Bipedal Walking On Planned Footsteps For Humanoid Robots

## Main Point

- Train one humanoid walking policy to follow planned footsteps.
- The policy only needs the next `2` footstep targets, not a full motion demo.
- Works for forward, backward, sideways, turning, stairs, and standing still.

## Key Idea

- Instead of saying "walk at this velocity", tell the robot "step here next".
- Each footstep target gives position plus heading: `x, y, z, yaw`.
- This connects RL control with classical footstep planners.

## Walking Rhythm

- The policy gets a clock signal that says which foot should swing or support.
- Single-support = one foot on ground, other foot moving.
- Double-support = both feet on ground between steps.
- This replaces motion-capture reference walking.

## Policy Setup

- Inputs: joint states, body balance info, clock signal, and next `2` footsteps.
- Output: target joint positions for the legs.
- A low-level PD controller turns those targets into torques.
- Actions are offsets from a half-sitting pose, which keeps knees bent and stable.

## Rewards

- Reward foot placement near the target.
- Reward body progress toward the target.
- Reward facing the target heading.
- Penalize jerky torques/actions and bad posture.

## Curriculum

- Start with flat walking.
- Later add stairs slowly, up to about `10cm`.
- This prevents the robot from learning to just stand still.

## Results

- Trained in MuJoCo on CPU, around `12` hours for `50M` samples.
- One policy can walk in many directions and climb/descend stairs in simulation.
- Handles about `+/-3cm` stair height noise before performance drops.

## Limitations

- No real-robot deployment shown.
- Backward walking is hard to learn together with other modes.
- Stairs plus turning were not fully trained.
- Needs hand-designed gait timing, reward structure, and curriculum.

## Remember

- Previous paper = speed from massive GPU parallelism.
- This paper = control interface: walking by following planned footsteps.

# Benchmarking Potential Based Rewards for Learning Humanoid Locomotion

## Main Point

- This paper studies reward shaping for humanoid running.
- PBRS does not make training much faster.
- PBRS does make reward weights much easier to tune.

## Problem

- Normal reward shaping gives rewards for being in a good state.
- If the weight is too high, the robot over-focuses on that reward.
- Example: too much upright reward can make the robot stiff instead of fast.

## DRS vs PBRS

- DRS = direct reward shaping: reward the current state.
- PBRS = potential-based reward shaping: reward improvement from one state to the next.
- Simple idea: reward "getting better", not "being perfect forever".

## Key Formula

- PBRS reward:

```text
P = gamma * Phi(next_state) - Phi(current_state)
```

- In practice, they used `gamma = 1` because it was more stable.

## Why PBRS Helps

- DRS keeps pushing the robot toward the shaping goal every timestep.
- PBRS fades away once the robot stops improving.
- This means PBRS guides learning early, then gets out of the way.

## Setup

- Robot: MIT Humanoid.
- Task: run with commanded forward, sideways, and turning velocities.
- Training: Isaac Gym, PPO, `4096` parallel agents.
- Short rollout horizon: `24` steps.
- Training takes about `30` minutes.

## Results

- PBRS, DRS, and baseline all learn in about the same time.
- PBRS has lower variance and slightly better final task reward.
- Biggest win: PBRS works across a much wider reward-weight range.
- DRS often fails when shaping weights are too large.

## Important Takeaway

- PBRS is not mainly a speed trick.
- PBRS is a reward-tuning trick.
- Use PBRS for helper rewards like height, posture, or joint regularization.
- Keep core task rewards separate, like velocity tracking.

## Limitations

- Only tested in simulation.
- Only tested one main task: flat-ground humanoid running.
- PBRS helps shaping rewards, but the main task reward still needs tuning.

## Remember

- First paper = faster training with GPU parallelism.
- Second paper = humanoid control through planned footsteps.
- This paper = easier reward tuning using PBRS.

# Starter Repo: SB3 PPO on Humanoid-v4

## What It Is

- Not a research paper.
- A small starter repo for training PPO on Gymnasium `Humanoid-v4`.
- Uses Stable Baselines3's built-in PPO.
- Good for learning the basic training loop.

## Main Code Idea

- Create the `Humanoid-v4` environment.
- Wrap it with parallel CPU environments.
- Train PPO for a few million timesteps.
- Save checkpoints and run evaluation rollouts.

## What Humanoid-v4 Means

- It is a standard MuJoCo benchmark humanoid.
- The reward is built in: move forward, stay alive, use less energy.
- It is not designed for natural walking or real-robot transfer.
- The learned gait may work but often looks awkward.

## PPO Setup

- Uses around `8` CPU environments.
- Uses `n_steps = 2048`.
- Total batch size is about `8 x 2048 = 16384`.
- Mini-batch size is only `64`.
- This is simple SB3-style PPO, not research-grade locomotion PPO.

## Compared To The Papers

- Much less parallelism than Isaac Gym papers using `4096` robots.
- No custom reward design.
- No curriculum.
- No clock signal.
- No footstep targets.
- No domain randomization.
- No sim-to-real setup.
- No timeout bootstrapping fix.

## Why It Is Still Useful

- Good "hello world" for humanoid RL.
- Helps you understand PPO, vectorized environments, checkpoints, and evaluation.
- Useful before building a custom locomotion pipeline.

## If Improving It

- First improve the reward function.
- Add velocity tracking, action smoothness, and torque penalties.
- Add a clock signal for cleaner biped rhythm.
- Increase parallelism if possible.
- Add curriculum learning after the basic policy works.
- Add domain randomization if you care about robustness.

## Remember

- This repo teaches the mechanics of training PPO.
- The research papers teach what makes locomotion actually work well.

# Medium Tutorial: Isaac Lab Humanoid Target Walking

## What It Is

- Not a research paper.
- A beginner tutorial for setting up a humanoid RL project in Isaac Lab.
- Uses an Isaac Lab template and modifies a stock humanoid example.
- Goal: train a humanoid to move to a target, stop, and balance.

## Why It Is Useful

- Good practical intro to Isaac Lab project structure.
- Shows where environment code and config code live.
- Helps connect theory papers to an actual training project.

## Main Files

- `humanoid_target_move_env.py` defines environment logic.
- `humanoid_target_move_env_cfg.py` defines config, rewards, start positions, and robot setup.

## Remember

- This is a getting-started guide.
- Useful for learning Isaac Lab mechanics.
- Not useful as a novel locomotion research contribution.

# Humanoid-Gym

## Main Point

- Open-source framework for training humanoid walking policies.
- Built on Isaac Gym, `legged_gym`, and `rsl_rl`.
- Goal: train in Isaac Gym, test in MuJoCo, deploy on real humanoids.

## Key Idea

- Isaac Gym is fast for training.
- MuJoCo is used as a more realistic sim-to-sim check.
- If a policy works in both simulators, it is more likely to work on hardware.

## Pipeline

- Train with thousands of parallel humanoids in Isaac Gym.
- Validate the same policy in MuJoCo.
- Deploy to real robots with zero-shot sim-to-real transfer.

## Policy Setup

- Actor sees only deployable sensor info.
- Critic sees extra privileged sim info during training.
- This is called asymmetric actor-critic.
- Actor uses frame stacking, so it gets recent history instead of one frame.

## Gait Design

- Uses a clock signal for walking phase.
- Uses stance masks to say which foot should contact the ground.
- Uses simple reference joint motions to guide the gait.
- This is more structured than learning completely from scratch.

## Rewards

- Track commanded forward, sideways, and turning velocity.
- Match expected foot contact pattern.
- Track reference joint motion.
- Keep body stable and at good height.
- Penalize energy use, rough actions, and excessive contact force.

## Domain Randomization

- Randomizes friction, motor strength, payload, sensor noise, and delay.
- Delay randomization is important because real humanoid control has latency.
- Purpose: make the policy robust before real deployment.

## Training Details

- Uses about `8192` parallel environments.
- Uses short rollouts of about `24` steps.
- Batch size is about `8192 x 24 = 196608`.
- This is very close to the massive-parallelism ideas from Rudin et al.

## Results

- Shows zero-shot transfer to real XBot humanoids.
- Works on two robot sizes: smaller XBot-S and larger XBot-L.
- Main evidence is qualitative: videos, sim-to-sim checks, and real walking.

## Limitations

- Few quantitative metrics.
- No strong ablation studies.
- Real-world tests are mostly flat-ground walking.
- No perception or terrain height map.
- Still tied to Isaac Gym, which is older than Isaac Lab.

## Remember

- Rudin paper = massive parallel training recipe.
- Humanoid-Gym = adapts that recipe to humanoids.
- Medium tutorial = beginner Isaac Lab setup.
- Humanoid-Gym paper = practical humanoid sim-to-real framework.
