# Humanoid Locomotion Training Experience

## Slide Outline

### Slide 1: Goal

- Train a MuJoCo `Humanoid-v5` agent to walk using PPO.
- Start with the default environment reward.
- Add custom rewards step by step.
- Final goal explored: stylized Naruto-like running.

Speaker notes:
The goal was not just to get reward high, but to understand how reward design affects locomotion behavior. I started with the built-in MuJoCo humanoid task, then gradually added shaping rewards based on what I observed visually.

Image idea:
- Screenshot of the baseline humanoid standing/falling.
- Screenshot of later gait policy alternating feet.

---

### Slide 2: Baseline PPO Setup

- Environment: Gymnasium `Humanoid-v5`.
- Algorithm: PPO from Stable Baselines3.
- Parallel environments: `8`.
- Observation/reward normalization: `VecNormalize`.
- Initial training: default MuJoCo reward.

Speaker notes:
I first used the built-in reward because I wanted a clean baseline. The default reward encourages forward movement, staying alive, and not using excessive control. It does not explicitly reward natural walking, foot alternation, upright torso, or human-like style.

---

### Slide 3: Baseline Result

- The humanoid learned to move forward.
- But the gait was unnatural.
- One foot often stayed forward while the other dragged.
- Reward improved, but behavior was not human-like.

Speaker notes:
This showed the classic reward mismatch problem. The policy found a behavior that satisfied the environment reward, but not the behavior I wanted. This was a good example of why visual inspection matters in RL locomotion.

Image idea:
- Screenshot/video frame of the default policy dragging one leg.

---

### Slide 4: First Custom Reward: Smoothness

- Added action smoothness penalty.
- Penalized large changes between consecutive actions.
- Goal: reduce twitchy, unstable motion.
- This helped produce cleaner motion.

Reward idea:

```text
smoothness_cost = mean((action_t - action_t-1)^2)
reward -= smooth_weight * smoothness_cost
```

Speaker notes:
Smoothness was the safest first reward term. It does not force a specific gait, but it discourages violent joint commands. This is common in locomotion papers because real robots cannot use extremely jerky actions safely.

---

### Slide 5: Foot Clearance Experiment

- Added reward for lifting the moving foot.
- Goal: reduce foot dragging.
- Result: not much better than smoothness alone.
- Sometimes reward increased without visibly better walking.

Speaker notes:
Foot clearance seemed like the obvious fix for dragging, but in practice it did not solve the real problem. The robot needed better step timing and foot placement, not just foot height. This was an important lesson: a reward that sounds correct may not target the actual failure mode.

---

### Slide 6: Gait Timing Reward

- Added a walking clock.
- First half of cycle: one foot supports, other swings.
- Second half: switch feet.
- Added clock to observation using `sin` and `cos`.
- Result: the robot started alternating feet.

Reward idea:

```text
phase < 0.5: left support, right swing
phase >= 0.5: right support, left swing
```

Speaker notes:
This was the biggest improvement. Without a clock, the robot had no reason to alternate feet. The gait clock gave the policy a rhythm, similar to ideas from humanoid locomotion papers that use phase variables or periodic reward composition.

Image idea:
- Diagram: left support/right swing, then right support/left swing.

---

### Slide 7: Tuning Gait Speed

- Fast gait cycle caused tiny, rapid steps.
- Slow gait cycle caused limping.
- Middle value worked better.
- Gait cadence became a key hyperparameter.

Speaker notes:
I experimented with the gait cycle length. A very fast clock made the robot rapidly alternate feet with small unnatural steps. A very slow clock made it limp because it held one support phase too long. A middle cadence worked better.

---

### Slide 8: Step-Forward Reward

- Contact timing alone was not enough.
- The robot alternated contact but kept one foot forward.
- Added reward for the swing foot moving ahead of the pelvis.
- This targeted actual step placement.

Reward idea:

```text
if right foot is swinging:
    reward right_foot_x > pelvis_x
if left foot is swinging:
    reward left_foot_x > pelvis_x
```

Speaker notes:
This was a useful correction. Contact timing tells the robot which foot should touch the ground, but it does not tell the swing foot where to go. The step-forward reward encouraged actual left-right stepping.

---

### Slide 9: Naruto Run Attempt

- Added reward for arms behind the torso.
- Later required both arms to be behind and stretched.
- Added mild forward-lean reward.
- Result: walking degraded and arms were not reliably learned.

Speaker notes:
This exposed a hard tradeoff. The humanoid used its arms and torso for balance. Forcing arms behind the body made balancing harder, and the policy found compromises like one arm back and one arm forward. This showed that style rewards can interfere with locomotion.

Image idea:
- Naruto run reference image.
- Screenshot of policy with one arm behind and poor gait.

---

### Slide 10: Mocap Exploration

- Downloaded a rigged Naruto running GLB.
- Inspected skeleton and animation with `pygltflib`.
- Found Mixamo-style bones and one running animation.
- Did not fully retarget to MuJoCo.

Speaker notes:
The mocap asset had a real skeleton and animation, but using it properly requires retargeting. The GLB has bones like `mixamorig:LeftArm`, while MuJoCo has joints like `left_shoulder1`, `left_shoulder2`, and `left_elbow`. Mapping between them is non-trivial.

---

### Slide 11: Key Lessons

- Default reward can produce movement, not necessarily natural walking.
- Smoothness helped.
- Gait clock helped more.
- Foot clearance alone was not enough.
- Style rewards can break balance.
- Mocap requires retargeting before full imitation.

Speaker notes:
The main lesson was that locomotion is not just about high reward. You need reward terms that match the behavior you actually want, and each term can create new failure modes.

---

## Detailed Interview Explanation

## 1. What Were You Trying To Build?

I was training a simulated humanoid in MuJoCo to walk using reinforcement learning. I used Gymnasium's `Humanoid-v5` environment and Stable Baselines3 PPO. The project started as a baseline locomotion experiment and then evolved into custom reward engineering for more structured gait behavior.

The long-term fun goal was a Naruto-style run, but the more serious learning goal was understanding how humanoid locomotion rewards shape behavior.

## 2. Why Start With The Default MuJoCo Reward?

I started with the built-in reward because it gave me a clean baseline. Before designing custom rewards, I wanted to know what PPO could learn from the standard task.

The default `Humanoid-v5` reward mainly encourages:

- forward movement
- staying alive
- minimizing excessive control effort

It does not explicitly encourage:

- alternating left/right steps
- upright human-like posture
- foot clearance
- symmetry
- natural arm motion
- a specific gait rhythm

So it was useful as a first benchmark, but not enough for human-like walking.

## 3. What Happened With The Baseline?

The baseline eventually learned to move forward, but the behavior was not natural. It often dragged one leg or kept one foot ahead while the other trailed behind.

This showed that the policy was optimizing the reward, but the reward was not specific enough. It learned a valid way to get reward, not necessarily the movement I wanted.

This is a form of reward mismatch. I would not call it extreme reward hacking, but the policy exploited the looseness of the default reward.

## 4. Why Use PPO?

PPO is a common choice for locomotion because it is stable, widely used, and works well with continuous control. It is also the algorithm used or inherited by many locomotion systems, including legged locomotion papers and frameworks.

PPO alternates between:

1. collecting experience from the current policy
2. updating the policy with clipped gradient updates

The clipping prevents the policy from changing too much at once, which is useful for unstable tasks like humanoid walking.

## 5. Why Use `VecNormalize`?

The humanoid observation contains values at very different scales:

- joint angles
- joint velocities
- body velocities
- contact-related values
- torso positions

Neural networks train better when inputs are normalized. `VecNormalize` tracks the running mean and standard deviation of observations and rewards, then rescales them.

This makes PPO training more stable.

Important detail:

```text
If you train with VecNormalize, you must save and reload the normalization stats.
```

Otherwise, during evaluation, the policy sees observations on a different scale and may fail.

## 6. Why These PPO Hyperparameters?

The setup used understandable, conservative PPO settings:

- `n_steps = 512`
- `batch_size = 1024`
- `n_epochs = 5`
- `gamma = 0.99`
- `gae_lambda = 0.95`
- `learning_rate = 3e-4`
- `clip_range = 0.2`
- `target_kl = 0.03`
- network size `[256, 256]`

Explanation:

- `n_steps=512`: collects enough temporal experience before each update.
- `batch_size=1024`: splits rollout data into mini-batches for training.
- `n_epochs=5`: reuses each collected batch several times.
- `gamma=0.99`: walking needs future reward, so the agent must care about long-term consequences.
- `gae_lambda=0.95`: standard GAE tradeoff between bias and variance.
- `learning_rate=3e-4`: common PPO starting point.
- `clip_range=0.2`: standard PPO clipping.
- `target_kl`: safety stop if the policy changes too much.
- `[256, 256]`: large enough for humanoid control but still simple.

I also observed frequent KL early stopping, which meant the policy updates were aggressive. That is something I would tune later with a smaller learning rate.

## 7. Why Add Smoothness First?

The first custom reward was action smoothness:

```text
smoothness_cost = mean((action_t - action_t-1)^2)
reward -= weight * smoothness_cost
```

I chose this first because it is low-risk and common in robotics. It does not force a specific gait, but it discourages sudden joint command changes.

This matters because:

- jerky actions look bad
- real robots cannot safely execute violent commands
- smoother control often improves stability

Smoothness improved the behavior and became one of the best-performing custom additions.

## 8. Why Did Foot Clearance Not Help Much?

Foot clearance means rewarding the swing foot for lifting off the ground instead of dragging.

The idea was:

```text
reward foot moving forward while slightly lifted
```

This seemed directly related to the dragging-foot problem, but it did not help much.

Why?

Because the real issue was not only foot height. The robot did not have a reliable left-right stepping rhythm or foot placement strategy. Rewarding height alone does not teach:

- when to swing each foot
- where the foot should land
- how to shift weight
- how to coordinate torso and pelvis

So foot clearance was not wrong, but it was not the main missing structure.

## 9. What Is Gait?

A gait is the repeated movement pattern used for locomotion.

For walking:

```text
left foot supports -> right foot swings
brief transition
right foot supports -> left foot swings
brief transition
repeat
```

Gait includes:

- which foot is on the ground
- which foot is moving
- how long each phase lasts
- where each foot lands
- how the torso balances

The default reward did not define a gait, so the robot invented weird movement patterns.

## 10. Why Add A Gait Clock?

The gait clock gave the robot a phase signal:

```text
phase from 0 to 1
```

I added it to the observation as:

```text
sin(2π phase), cos(2π phase)
```

This avoids a discontinuity when the phase wraps from `1` back to `0`.

The reward used the phase to define expected contact:

```text
phase < 0.5:
    left foot supports
    right foot swings

phase >= 0.5:
    right foot supports
    left foot swings
```

This was the first reward that really encouraged alternating feet.

## 11. Why Did Gait Cycle Length Matter?

The gait cycle length controlled cadence.

When it was too fast, the robot took tiny rapid steps.

When it was too slow, the robot started limping because it stayed in one support phase too long.

This showed that gait timing is a real hyperparameter, not just a cosmetic detail.

The best behavior came from an intermediate gait timing.

## 12. Why Add Swing-Foot Forward Reward?

The contact reward alone taught:

```text
which foot should touch the ground
```

But it did not teach:

```text
the swing foot should move forward
```

So the robot could alternate contacts while still keeping one foot mostly in front.

I added:

```text
if right foot is swinging:
    reward right foot ahead of pelvis

if left foot is swinging:
    reward left foot ahead of pelvis
```

This targeted actual step placement.

## 13. Why Did Naruto Running Fail?

Naruto running is hard because it is a style constraint on top of a balance problem.

A humanoid uses arms and torso for balance. When I rewarded arms behind the body, the policy often sacrificed gait quality or exploited the reward by putting one arm back and keeping the other arm available for balance.

The conflict was:

```text
balance wants arms free
Naruto style wants arms locked behind
forward movement wants speed
reward optimization finds loopholes
```

So the policy did not learn clean Naruto running from simple hand-position rewards.

## 14. Why Did One Arm Go Back But Not Both?

The first arms-back reward averaged both arms:

```text
reward = 0.5 * (right_score + left_score)
```

This allowed an exploit:

```text
one arm good, one arm bad -> still gets partial reward
```

A stricter version used:

```text
reward = min(right_score, left_score)
```

That requires both arms to satisfy the reward. However, even that made balancing harder.

## 15. Why Not Just Use Mocap Directly?

Mocap gives a reference human animation. I downloaded a rigged Naruto running GLB and inspected it with `pygltflib`.

It had:

- Mixamo-style skeleton
- one running animation
- animated arms, spine, legs, feet
- duration around `0.63s`

But direct imitation requires retargeting.

The mocap skeleton has bones like:

```text
mixamorig:LeftArm
mixamorig:LeftForeArm
mixamorig:Hips
```

MuJoCo has joints like:

```text
left_shoulder1
left_shoulder2
left_elbow
abdomen_y
right_hip_y
right_knee
```

Retargeting means converting the mocap bone animation into MuJoCo joint targets. That is non-trivial because the skeletons, coordinate systems, joint axes, and proportions differ.

## 16. What Did Mocap Teach Us Anyway?

Even without full retargeting, the mocap helped identify style targets:

- forward torso lean
- arms rotated back
- forearms extended
- fast gait cadence

So I tried a hand-mapped mocap-style reward. It was closer to imitation, but still not full retargeting. It did not solve Naruto running because the mapping was too crude.

## 17. What Worked Best?

The best practical model was the smooth gait version:

```text
MuJoCo default reward
- action smoothness penalty
+ gait contact timing reward
+ swing-foot-forward reward
+ upright torso reward
```

This produced the most understandable improvement:

- smoother motion
- visible left-right alternation
- better walking structure

It was still not perfectly human-like, but it was the best learning result.

## 18. What Would You Do Next?

If continuing seriously, I would do one of two paths.

Path 1: Better reward engineering

- stronger torso posture reward
- pelvis height reward
- head-over-pelvis reward
- symmetry reward
- step length target
- tuned gait cadence

Path 2: Proper imitation learning

- retarget mocap to MuJoCo humanoid
- generate reference `qpos` trajectory
- train policy with pose imitation reward
- combine imitation with stability and forward velocity rewards

For true Naruto running, I would choose path 2.

## Reward Function Details

This section is the part I would use to explain the project technically in an interview. The main idea is that I did not design one giant reward immediately. I started with the default MuJoCo reward, watched the failure mode, then added one shaping term at a time.

## Reward Stack Overview

The best-performing direction was:

```text
final_reward =
    MuJoCo default reward
  - smoothness penalty
  + gait contact timing reward
  + swing-foot-forward reward
  + upright torso reward
```

The default MuJoCo reward handled the broad task:

```text
move forward + stay alive - control cost
```

My custom terms handled the behavior I cared about:

```text
move smoothly
alternate feet
place the swing foot forward
keep torso more upright
```

## 1. Smoothness Reward

Code shape:

```python
class SmoothReward(gym.Wrapper):
    def __init__(self, env, weight=0.08):
        super().__init__(env)
        self.weight = weight
        self.prev_action = np.zeros(env.action_space.shape, dtype=np.float32)

    def reset(self, **kwargs):
        self.prev_action[:] = 0.0
        return self.env.reset(**kwargs)

    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)

        smoothness_cost = np.square(action - self.prev_action).mean()
        reward = reward - self.weight * smoothness_cost
        self.prev_action = action.copy()

        info["custom/smoothness_cost"] = smoothness_cost
        return obs, reward, terminated, truncated, info
```

What it does:

```text
penalizes sudden action changes
```

The action is the vector of joint commands. If the current action is very different from the previous action, the robot is probably moving jerkily.

Why I chose it:

- It is simple.
- It is common in robot locomotion.
- It reduces twitchy policies.
- It does not force a specific gait.
- It is safer than adding a strong style reward early.

Benefit:

- Cleaner motion.
- Less violent joint behavior.
- Better chance of transfer to real robots later.

Con:

- If the weight is too high, the robot may become too timid.
- It can discourage fast running if overused.

How I would explain it:

I used smoothness first because I wanted to improve control quality without over-constraining the behavior. It is a regularizer for the policy's motor commands.

## 2. Foot Clearance Reward

Code shape:

```python
def _clearance_reward(self):
    data = self.unwrapped.data

    right_height = data.xpos[self.right_foot_id][2]
    left_height = data.xpos[self.left_foot_id][2]

    right_forward_vel = data.cvel[self.right_foot_id][3]
    left_forward_vel = data.cvel[self.left_foot_id][3]

    target_height = 0.08

    right_lift_score = np.exp(-40.0 * (right_height - target_height) ** 2)
    left_lift_score = np.exp(-40.0 * (left_height - target_height) ** 2)

    return (
        max(right_forward_vel, 0.0) * right_lift_score
        + max(left_forward_vel, 0.0) * left_lift_score
    )
```

What it does:

```text
rewards a foot for being slightly lifted while moving forward
```

Why not just reward foot height?

Because the robot might learn to kick or hop. I only wanted reward when the foot was moving forward, because that is closer to a real swing phase.

Why I chose it:

- The baseline dragged a foot.
- Foot clearance sounded like the direct fix.
- It is used in locomotion reward design to prevent scraping.

Benefit:

- Can reduce foot dragging.
- Encourages swing-leg behavior.

Con:

- It did not solve the main issue in my experiments.
- It can reward weird kicking if weighted too strongly.
- It does not teach when to swing each foot.

What I learned:

Foot clearance was not the main missing piece. The robot needed gait timing and foot placement more than just foot height.

## 3. Gait Clock Observation

Code shape:

```python
def _add_clock(self, obs):
    angle = 2.0 * np.pi * self.phase
    clock = np.array([np.sin(angle), np.cos(angle)], dtype=obs.dtype)
    return np.concatenate([obs, clock])
```

What it does:

```text
adds two numbers to the observation that tell the policy where it is in the gait cycle
```

Why `sin` and `cos`?

A raw phase jumps from `1` back to `0`. That discontinuity can confuse the policy. `sin` and `cos` make the phase wrap smoothly.

Example:

```text
phase 0.00 -> clock = (0, 1)
phase 0.25 -> clock = (1, 0)
phase 0.50 -> clock = (0, -1)
phase 1.00 -> clock = (0, 1)
```

Why I chose it:

- Walking is periodic.
- The robot needed a rhythm.
- Research papers often use phase/clock signals for bipedal gaits.

Benefit:

- Gives the policy timing information.
- Makes left-right alternation easier to learn.

Con:

- The gait cycle speed becomes a hyperparameter.
- If the clock is too fast, steps become tiny and frantic.
- If the clock is too slow, the robot can limp.

## 4. Contact Timing Reward

Code shape:

```python
def _contact_reward(self):
    left_contact = self._foot_touching_floor(self.left_foot_geom_id)
    right_contact = self._foot_touching_floor(self.right_foot_geom_id)

    if self.phase < 0.5:
        expected_left_contact = True
        expected_right_contact = False
    else:
        expected_left_contact = False
        expected_right_contact = True

    left_match = left_contact == expected_left_contact
    right_match = right_contact == expected_right_contact
    return 0.5 * float(left_match) + 0.5 * float(right_match)
```

What it does:

```text
rewards the correct foot contact pattern for the current gait phase
```

Interpretation:

```text
phase < 0.5:
    left foot should support
    right foot should swing

phase >= 0.5:
    right foot should support
    left foot should swing
```

Why I chose it:

The baseline did not alternate feet cleanly. Contact timing directly teaches the structure of walking.

Benefit:

- Encourages left-right alternation.
- Prevents one foot from doing all the work.
- Makes the behavior more gait-like.

Con:

- It can produce contact alternation without good foot placement.
- It can be too rigid if weighted too strongly.
- It does not by itself say where the swing foot should go.

What I learned:

This was one of the most important reward terms. It changed the behavior from "find any way forward" to "follow a walking rhythm."

## 5. Swing-Foot-Forward Reward

Code shape:

```python
def _step_reward(self):
    data = self.unwrapped.data
    pelvis_x = data.xpos[self.pelvis_body_id][0]
    right_foot_x = data.xpos[self.right_foot_body_id][0]
    left_foot_x = data.xpos[self.left_foot_body_id][0]

    if self.phase < 0.5:
        swing_foot_ahead = right_foot_x - pelvis_x
    else:
        swing_foot_ahead = left_foot_x - pelvis_x

    return min(max(float(swing_foot_ahead), 0.0), 0.5)
```

What it does:

```text
rewards the swing foot for moving ahead of the pelvis
```

Why I added it:

The contact reward taught alternating contact, but the robot could still keep one foot mostly forward. It needed a reward for actual step placement.

Benefit:

- Encourages real stepping.
- Connects swing phase to forward progress.
- Makes the contact timing reward more meaningful.

Con:

- If too strong, the robot may overreach.
- It assumes forward walking.
- It does not handle turning or sideways walking.

How I would explain it:

Contact timing says which foot should be on the ground. Step reward says where the swing foot should move. Both are needed for a useful walking gait.

## 6. Upright Torso Reward

Code shape:

```python
def _upright_reward(self):
    mat = self.unwrapped.data.xmat[self.torso_body_id].reshape(3, 3)
    torso_up_dot_world_up = mat[2, 2]
    return max(float(torso_up_dot_world_up), 0.0)
```

What it does:

```text
rewards the torso for staying aligned with the world up direction
```

If the torso is upright, the dot product is close to `1`. If the torso tilts, it decreases.

Why I chose it:

The humanoid often leaned or stood at an angle. I needed a posture signal that encouraged vertical balance.

Benefit:

- Better posture.
- Less sideways leaning.
- More human-like walking.

Con:

- If too strong, the robot may become stiff.
- It can fight forward lean needed for running.
- For Naruto running, upright torso conflicts with the desired style.

What I learned:

Posture rewards are useful but delicate. Walking wants upright posture, but running and stylized motion may require lean.

## 7. Arms-Back Reward

Code shape:

```python
right_back = max(float(torso_x - right_hand[0]), 0.0)
left_back = max(float(torso_x - left_hand[0]), 0.0)

right_back_score = min(right_back / target_back_distance, 1.0)
left_back_score = min(left_back / target_back_distance, 1.0)

right_stretch_score = min(norm(right_hand - right_shoulder) / target_stretch, 1.0)
left_stretch_score = min(norm(left_hand - left_shoulder) / target_stretch, 1.0)

right_score = right_back_score * right_stretch_score
left_score = left_back_score * left_stretch_score

arms_reward = min(right_score, left_score)
```

What it does:

```text
rewards both hands being behind the torso and stretched away from the shoulders
```

Why use `min` instead of average?

With average:

```text
one arm good + one arm bad = still partial reward
```

With `min`:

```text
both arms must satisfy the target
```

Why I chose it:

The Naruto style requires both arms stretched behind the body.

Benefit:

- More specific than just "hands behind torso."
- Prevents the one-arm exploit.

Con:

- Strongly interfered with balance.
- The humanoid used arms for stabilization.
- The policy degraded its gait when forced toward the style.

What I learned:

Style rewards can conflict with locomotion rewards. Arms-back running is not just a pose problem; it changes the balance problem.

## 8. Forward Lean Reward

Code shape:

```python
forward_lean = max(float(torso_x - pelvis_x), 0.0)
lean_reward = np.exp(-20.0 * (forward_lean - target_lean) ** 2)
```

What it does:

```text
rewards the torso being slightly ahead of the pelvis
```

Why I chose it:

The Naruto reference has forward torso lean. The mocap inspection also showed spine rotation.

Benefit:

- Moves the policy toward a running-like pose.
- Useful for stylized locomotion.

Con:

- Too much lean causes falling.
- It conflicts with upright walking.
- It needs careful balance with posture reward.

What I learned:

Forward lean is a style feature, but in physics it is also a stability risk.

## 9. Mocap-Style Reward

Code shape:

```python
style_reward =
    0.35 * arms_back_reward
  + 0.25 * forward_lean_reward
  + 0.40 * leg_reference_reward
```

What it tried to do:

```text
imitate high-level Naruto style without full retargeting
```

The leg reference used phase-based target joint angles:

```python
if phase < 0.5:
    right leg = swing-like target
    left leg = support-like target
else:
    left leg = swing-like target
    right leg = support-like target
```

Why I tried it:

The GLB mocap showed that Naruto running is a coordinated full-body motion. I wanted to move from crude hand-position rewards toward a reference-pose reward.

Benefit:

- More structured than simple arms-back reward.
- Includes torso, arms, and legs.
- Closer to imitation learning conceptually.

Con:

- Still not true mocap retargeting.
- Joint targets were hand-mapped and approximate.
- It did not produce reliable Naruto running.

What I learned:

To use mocap properly, I need retargeting from the Mixamo skeleton to the MuJoCo humanoid joint space.

## Why The Final Best Reward Was Not The Most Complex

The best result was not the Naruto or mocap-style reward. The best practical walking result was the simpler smooth gait reward.

Reason:

```text
simple rewards aligned with the basic locomotion task
complex style rewards introduced conflicts
```

The smooth gait reward improved the actual walking structure without overloading the policy with style constraints.

This was an important engineering lesson:

```text
More reward terms do not automatically mean better behavior.
```

## Interview Questions And Answers

## Q: Why did you not start with a custom reward immediately?

A: I wanted a baseline first. Without a baseline, I would not know whether my custom rewards were actually improving behavior. The default reward established that PPO could learn some forward locomotion, but also showed the failure modes.

## Q: What was the biggest improvement?

A: The gait clock and contact timing reward. Smoothness made motion cleaner, but the gait clock changed the structure of the behavior by encouraging left-right alternation.

## Q: Why did foot clearance not solve dragging?

A: Because dragging was a symptom, not the root cause. The root problem was missing gait structure. The robot needed timing and foot placement, not just a reward for lifting the foot.

## Q: How did you choose reward weights?

A: I started small to avoid overpowering the default task reward. If a reward term is too strong, the agent can over-optimize it and sacrifice locomotion. I looked at both training metrics and visual behavior.

## Q: What metrics did you watch?

A: I watched:

- `ep_len_mean`: survival length
- `ep_rew_mean`: total reward trend
- visual gait quality
- whether it alternated feet
- whether torso posture improved
- whether reward increased while behavior got worse

Visual inspection was essential because high reward did not always mean better walking.

## Q: What is reward hacking in this project?

A: An example was the arms-back reward. The robot could put one arm behind and keep the other arm forward for balance, getting partial style reward without actually performing the desired pose.

## Q: Why did Naruto running break locomotion?

A: Because arms are part of the balance strategy. Forcing arms back reduced the robot's ability to stabilize. The style reward conflicted with the locomotion objective.

## Q: Why is humanoid walking hard?

A: Humanoids have many degrees of freedom, unstable contacts, high center of mass, and delayed consequences. Good walking requires coordinated sequences of actions, not just individual good actions.

## Q: Why use a clock signal?

A: A clock gives the policy phase information. Without phase, the policy has no explicit concept of which part of the gait cycle it is in. The clock helps coordinate periodic behavior.

## Q: Why use `sin` and `cos` for phase?

A: Because phase wraps around. If I used a raw phase value, it would jump from `1` to `0`. `sin` and `cos` make the phase continuous.

## Q: What would proper mocap imitation require?

A: Retargeting. I would need to convert the GLB/Mixamo skeleton animation into MuJoCo humanoid joint trajectories. Then I could reward the robot for matching those reference poses while still staying balanced.

## Q: Why not deploy to Isaac Sim immediately?

A: Sim2sim deployment requires matching the robot model, action order, observation vector, control frequency, and normalization. The policy was trained on MuJoCo `Humanoid-v5`, so Isaac would need to reproduce that exact interface.

## Q: What did you learn overall?

A: I learned that locomotion reward design is iterative. The agent will optimize exactly what you reward, often in unexpected ways. The best improvements came from adding structure gradually and testing visually after each change.

## Suggested Slide Images

- Baseline humanoid falling/standing.
- Baseline weird gait with one leg dragging.
- Diagram of gait clock phases.
- Screenshot of improved alternating gait.
- Screenshot of Naruto reward failure.
- Naruto reference image.
- GLB skeleton/bone list screenshot.
- Simple reward stack diagram.

## Final Summary

The project started with a default PPO humanoid baseline and gradually added custom reward terms. Smoothness improved stability, gait timing created alternating steps, and swing-foot-forward reward improved step placement. Attempts to force Naruto-style arms showed how style rewards can conflict with balance and create reward exploits. Mocap offered a better long-term direction, but would require retargeting before it could be used as a proper imitation reference.
