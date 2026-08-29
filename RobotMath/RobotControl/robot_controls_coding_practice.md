# Robot Controls Coding Practice

Use this as a fast interview drill for a 1-hour robotics controls coding assessment. The questions go from easy to hard and are written like coding prompts. Most can be solved in Python quickly, but several translate naturally to C++.

For each problem, aim to write clean code, explain your assumptions, and mention how the code would behave on real hardware with noise, latency, saturation, and imperfect sensors.

## Quick Control Concepts To Remember

- A controller computes a command from error: `error = target - measured`.
- A PID controller uses proportional, integral, and derivative terms:
  - `P`: reacts to current error.
  - `I`: accumulates error over time to remove steady-state offset.
  - `D`: reacts to rate of change and adds damping.
- Real robots need limits: clamp commands, handle invalid `dt`, avoid integral windup, filter noisy measurements.
- Position control commands position or velocity to reduce position error.
- Velocity control commands acceleration, torque, or motor effort to reduce velocity error.
- Torque control directly commands effort and is closer to physical dynamics.
- Simulation-to-real issues often come from wrong units, wrong signs, delay, friction, backlash, saturation, and sensor noise.

---

## Easy

### 1. Clamp A Motor Command

Write a function:

```python
def clamp_command(command: float, min_cmd: float, max_cmd: float) -> float:
    ...
```

Return `command` limited to `[min_cmd, max_cmd]`.

Example:

```python
clamp_command(12.5, -10.0, 10.0) == 10.0
clamp_command(-11.0, -10.0, 10.0) == -10.0
clamp_command(3.0, -10.0, 10.0) == 3.0
```

What they are testing:

- Basic safety limits.
- Clean function design.
- Handling edge cases.

Follow-up:

- What should happen if `min_cmd > max_cmd`?

---

### 2. Compute Position Error

Write a function:

```python
def position_error(target: float, measured: float) -> float:
    ...
```

Return the position error for a 1D joint.

Example:

```python
position_error(1.5, 1.0) == 0.5
position_error(-1.0, 2.0) == -3.0
```

What they are testing:

- Sign convention.
- Whether you can explain that wrong signs cause unstable feedback.

Follow-up:

- If the motor moves farther away from the target, what might be wrong?

---

### 3. Proportional Controller

Write a proportional position controller:

```python
def p_controller(target_pos: float, measured_pos: float, kp: float) -> float:
    ...
```

Return the motor command.

Formula:

```text
command = kp * (target_pos - measured_pos)
```

Add command saturation:

```python
def p_controller_limited(target_pos, measured_pos, kp, max_abs_cmd):
    ...
```

What they are testing:

- Basic feedback control.
- Saturation.
- Understanding that high `kp` can cause overshoot or oscillation.

---

### 4. Velocity From Encoder Positions

Given encoder positions sampled at a fixed timestep, compute velocity estimates.

```python
def estimate_velocity(positions: list[float], dt: float) -> list[float]:
    ...
```

For positions `[0.0, 0.1, 0.4, 0.9]` and `dt = 0.1`, return:

```python
[1.0, 3.0, 5.0]
```

What they are testing:

- Finite differences.
- Correct use of `dt`.
- Handling empty or single-value input.

Follow-up:

- Why can velocity estimated from position be noisy?

---

### 5. Moving Average Filter

Write a moving average filter for noisy sensor values.

```python
def moving_average(values: list[float], window_size: int) -> list[float]:
    ...
```

Example:

```python
moving_average([1, 2, 3, 4], 2) == [1.0, 1.5, 2.5, 3.5]
```

For the first few values, average over the values available so far.

What they are testing:

- Noise filtering.
- Boundary conditions.
- Understanding that filtering can add delay.

---

## Medium

### 6. Implement A PID Controller Class

Implement:

```python
class PIDController:
    def __init__(self, kp, ki, kd, min_cmd=-float("inf"), max_cmd=float("inf")):
        ...

    def reset(self):
        ...

    def update(self, target, measured, dt):
        ...
```

Requirements:

- Use `error = target - measured`.
- Integral term accumulates `error * dt`.
- Derivative term uses `(error - previous_error) / dt`.
- Clamp output to `[min_cmd, max_cmd]`.
- Handle the first call when no previous error exists.
- Handle invalid `dt <= 0`.

What they are testing:

- Stateful controller design.
- Edge cases.
- Practical control implementation.

Follow-up:

- What is integral windup?
- How would you prevent it?

---

### 7. Add Anti-Windup To PID

Modify the PID controller so the integral term does not grow without bound.

Possible approaches:

- Clamp the integral state.
- Stop integrating when the output is saturated and error would push farther into saturation.

Function shape:

```python
class PIDController:
    def __init__(self, kp, ki, kd, min_cmd, max_cmd, min_integral, max_integral):
        ...
```

What they are testing:

- Real hardware awareness.
- Ability to improve a naive PID implementation.

Good explanation:

If the actuator is saturated, the integral term may keep accumulating even though the robot cannot respond. When the robot finally can move, the large integral term causes overshoot.

---

### 8. Angle Error With Wraparound

A robot joint rotates continuously. Angles are in radians. Write:

```python
def shortest_angle_error(target: float, measured: float) -> float:
    ...
```

Return the shortest signed angular error in `[-pi, pi]`.

Examples:

```python
target = 0.1
measured = 2 * pi - 0.1
error should be approximately 0.2
```

```python
target = 2 * pi - 0.1
measured = 0.1
error should be approximately -0.2
```

What they are testing:

- Circular state spaces.
- Avoiding long-way-around rotation.
- Robotics math basics.

---

### 9. Rate Limit A Command

Real motor commands should not jump instantly. Write:

```python
def rate_limit_command(previous_cmd: float, desired_cmd: float, max_rate: float, dt: float) -> float:
    ...
```

`max_rate` is command units per second.

Example:

```python
previous_cmd = 0.0
desired_cmd = 10.0
max_rate = 2.0
dt = 0.5
result == 1.0
```

What they are testing:

- Smooth command generation.
- Safety-conscious control.
- Correct time scaling.

---

### 10. Simulate A 1D Point Mass

Simulate a 1D mass controlled by a PD controller.

Dynamics:

```text
x_dot = v
v_dot = force / mass
force = kp * (target_x - x) + kd * (target_v - v)
```

Write:

```python
def simulate_pd_mass(x0, v0, target_x, mass, kp, kd, dt, steps):
    ...
```

Return lists of positions and velocities over time.

Requirements:

- Use Euler integration.
- Clamp force to a reasonable max force.
- Include input validation for `mass > 0` and `dt > 0`.

What they are testing:

- Simple dynamics.
- PD control.
- Simulation loop structure.

Follow-up:

- What happens when `kp` is high and `kd` is low?
- What happens when `kd` is high?

---

### 11. Detect Overshoot And Settling Time

Given a list of measured positions over time, target position, and `dt`, compute:

```python
def analyze_step_response(positions: list[float], target: float, dt: float, tolerance: float) -> dict:
    ...
```

Return:

```python
{
    "max_overshoot": ...,
    "settling_time": ...
}
```

Define settling time as the first time after which all remaining positions stay within `tolerance` of the target.

What they are testing:

- Control performance metrics.
- Careful array logic.
- Ability to evaluate tuning.

---

### 12. Low-Pass Filter A Sensor

Implement an exponential moving average:

```python
class LowPassFilter:
    def __init__(self, alpha):
        ...

    def update(self, measurement):
        ...

    def reset(self):
        ...
```

Formula:

```text
filtered = alpha * measurement + (1 - alpha) * previous_filtered
```

Requirements:

- `alpha` must be in `[0, 1]`.
- First update should return the measurement directly.

What they are testing:

- Filtering noisy sensor data.
- Stateful code.
- Understanding the tradeoff between smoothness and delay.

---

## Hard

### 13. Joint Controller With Position, Velocity, And Effort Limits

Implement a joint controller class:

```python
class JointController:
    def __init__(self, kp, kd, min_pos, max_pos, max_vel, max_effort):
        ...

    def compute_effort(self, target_pos, target_vel, measured_pos, measured_vel):
        ...
```

Requirements:

- Clamp target position to joint limits.
- Clamp target velocity to `[-max_vel, max_vel]`.
- Use PD control:

```text
effort = kp * (target_pos - measured_pos) + kd * (target_vel - measured_vel)
```

- Clamp effort to `[-max_effort, max_effort]`.

What they are testing:

- Combining control logic with physical limits.
- Clear readable class design.
- Practical robot joint control.

Follow-up:

- Why do physical limits belong near the controller?

---

### 14. Detect Encoder Direction Sign Error

You command a joint with a positive effort for several timesteps and record encoder positions.

Write:

```python
def detect_encoder_sign_error(commands: list[float], positions: list[float]) -> bool:
    ...
```

Return `True` if the encoder direction appears inverted.

Assume:

- Positive command should generally increase position.
- Ignore timesteps where command magnitude is near zero.
- Use the correlation/sign agreement between command and position delta.

What they are testing:

- Hardware debugging in code.
- Reasoning from logged data.
- Robustness to noise.

Follow-up:

- What else could make this test fail besides encoder sign?

---

### 15. Find Latency Between Command And Sensor Response

Given two same-length lists:

- `commands[t]`
- `positions[t]`

Estimate the delay in timesteps between a command change and sensor response.

Write:

```python
def estimate_delay_steps(commands: list[float], positions: list[float], max_delay: int) -> int:
    ...
```

Hint:

- Compute velocity from positions.
- Try delays from `0` to `max_delay`.
- Pick the delay with the best correlation between `commands[t]` and `velocity[t + delay]`.

What they are testing:

- Log analysis.
- Delay/latency awareness.
- Reasonable signal processing under time pressure.

Follow-up:

- Why does delay make feedback control unstable?

---

### 16. Impedance Control In 1D

Implement a 1D impedance controller:

```python
def impedance_control(target_x, target_v, measured_x, measured_v, stiffness, damping, max_force):
    ...
```

Formula:

```text
force = stiffness * (target_x - measured_x) + damping * (target_v - measured_v)
```

Clamp to `[-max_force, max_force]`.

Then simulate a point mass using this controller.

What they are testing:

- Conceptual impedance control.
- Understanding stiffness and damping.
- How "feel" can be shaped by gains.

Good explanation:

Higher stiffness makes the system resist position error more strongly. Higher damping resists velocity error and can make motion feel smoother or heavier.

---

### 17. Follow A Smooth Trajectory

Generate a smooth trajectory from `start` to `goal` in `duration` seconds using cubic smoothstep:

```text
s = t / duration
position = start + (goal - start) * (3s^2 - 2s^3)
```

Write:

```python
def generate_smooth_trajectory(start, goal, duration, dt):
    ...
```

Return a list of `(time, position, velocity)` tuples.

Requirements:

- Include `t = 0` and `t = duration`.
- Estimate or analytically compute velocity.
- Validate `duration > 0` and `dt > 0`.

What they are testing:

- Motion primitive generation.
- Smooth setpoints.
- Time-indexed control.

Follow-up:

- Why is a smooth trajectory better than commanding a step position?

---

### 18. Track A Trajectory With Feedforward Plus PD

Given a trajectory of desired position, velocity, and acceleration:

```python
trajectory = [
    {"pos": ..., "vel": ..., "acc": ...},
    ...
]
```

Write a controller:

```python
def track_trajectory(desired, measured_pos, measured_vel, mass, kp, kd, max_force):
    ...
```

Use:

```text
feedforward = mass * desired["acc"]
feedback = kp * (desired["pos"] - measured_pos) + kd * (desired["vel"] - measured_vel)
force = feedforward + feedback
```

What they are testing:

- Feedforward plus feedback.
- Basic model-based control.
- Understanding why feedback is still needed.

Follow-up:

- What happens if the mass estimate is wrong?

---

### 19. Build A Safety Monitor For A Joint

Implement:

```python
class JointSafetyMonitor:
    def __init__(self, min_pos, max_pos, max_vel, max_effort):
        ...

    def check(self, pos, vel, effort_cmd):
        ...
```

Return a dictionary:

```python
{
    "safe": bool,
    "violations": list[str]
}
```

Violations can include:

- `"position_low"`
- `"position_high"`
- `"velocity_limit"`
- `"effort_limit"`

What they are testing:

- Defensive robotics software.
- Clear interfaces.
- Safety-aware coding.

Follow-up:

- Should the safety monitor clamp commands, stop the robot, or only report problems?

---

### 20. Debug A Bad Controller From Logs

Given logs:

```python
logs = [
    {"t": 0.0, "target": 1.0, "pos": 0.0, "cmd": 5.0},
    {"t": 0.1, "target": 1.0, "pos": -0.2, "cmd": 6.0},
    {"t": 0.2, "target": 1.0, "pos": -0.5, "cmd": 7.5},
    {"t": 0.3, "target": 1.0, "pos": -0.9, "cmd": 9.5},
]
```

Write:

```python
def diagnose_control_log(logs: list[dict]) -> list[str]:
    ...
```

Return likely issues, such as:

- `"wrong_feedback_sign"`
- `"command_saturation"`
- `"overshoot"`
- `"tracking_error_increasing"`
- `"sensor_stuck"`

What they are testing:

- Robotics intuition from data.
- Practical debugging.
- Ability to convert observations into code.

Good reasoning:

Here the target is positive, command is positive, but position moves negative and error grows. That suggests a sign error in actuator direction, encoder direction, or feedback calculation.

---

## Very Hard

### 21. Tune PD Gains Automatically By Grid Search

Using your `simulate_pd_mass` function, search over candidate `kp` and `kd` values.

Write:

```python
def tune_pd_grid_search(kp_values, kd_values, sim_config):
    ...
```

Score each simulation using:

```text
score = final_error + 0.1 * max_overshoot + 0.01 * sum(abs(force))
```

Return the best `(kp, kd, score)`.

What they are testing:

- Optimization loop.
- Controller evaluation.
- Tradeoffs between accuracy, overshoot, and effort.

Follow-up:

- Why might grid-searched gains in simulation fail on hardware?

---

### 22. Extended State Estimator For Constant Velocity Motion

Implement a simple alpha-beta filter for 1D position and velocity estimation.

```python
class AlphaBetaFilter:
    def __init__(self, alpha, beta, initial_pos=0.0, initial_vel=0.0):
        ...

    def update(self, measured_pos, dt):
        ...
```

Prediction:

```text
pred_pos = pos + vel * dt
pred_vel = vel
```

Residual:

```text
residual = measured_pos - pred_pos
```

Correction:

```text
pos = pred_pos + alpha * residual
vel = pred_vel + beta * residual / dt
```

What they are testing:

- State estimation intuition.
- Filtering noisy encoder data.
- Similar thinking to a simplified Kalman filter.

Follow-up:

- What do alpha and beta control?

---

### 23. Multi-Joint Command Synchronization

You have several joints with different start and goal positions. Generate synchronized smooth trajectories so all joints start and finish together.

Write:

```python
def generate_multi_joint_trajectory(starts, goals, duration, dt):
    ...
```

Return:

```python
[
    {"t": 0.0, "positions": [...], "velocities": [...]},
    ...
]
```

Requirements:

- `starts` and `goals` must have same length.
- Use the same smoothstep phase `s` for all joints.
- Include velocity for each joint.

What they are testing:

- Coordinated motion.
- Array/list handling.
- Motion primitive design.

Follow-up:

- How would you enforce per-joint velocity limits?

---

### 24. Whole-Loop Robot Joint Simulation

Build a mini simulation combining:

- Smooth trajectory generation.
- PD or impedance controller.
- Point-mass or joint dynamics.
- Command saturation.
- Sensor noise.
- Low-pass filtering.
- Log analysis for overshoot and settling time.

Write:

```python
def run_joint_control_experiment(config: dict) -> dict:
    ...
```

Return:

```python
{
    "logs": [...],
    "metrics": {...},
    "diagnostics": [...]
}
```

What they are testing:

- End-to-end controls thinking.
- Code organization under time pressure.
- Ability to balance correctness and practical assumptions.

Interview strategy:

If this appears in a 1-hour assessment, implement the simplest working version first:

1. Generate setpoints.
2. Compute control command.
3. Simulate dynamics.
4. Log values.
5. Add metrics.
6. Add diagnostics only if time remains.

---

## Fast Practice Order If You Have 1 Hour

Do these first:

1. Problem 6: PID controller.
2. Problem 8: angle wraparound.
3. Problem 10: PD mass simulation.
4. Problem 13: joint controller with limits.
5. Problem 17: smooth trajectory.
6. Problem 20: diagnose logs.

If you can do those, you are in decent shape for a controls coding screen.

## Common Interview Mistakes

- Forgetting `dt` in integral, derivative, velocity, or rate-limit calculations.
- Not handling `dt <= 0`.
- Wrong sign convention for feedback.
- No command saturation.
- No reset method for stateful filters/controllers.
- Letting integral windup grow forever.
- Confusing degrees and radians.
- Ignoring angle wraparound.
- Writing code that works only for one hardcoded example.
- Not explaining real-world limitations like noise, latency, friction, backlash, and saturation.

## Tiny Answer Templates

When explaining a PID:

```text
I compute error as target minus measured. The proportional term reacts to current error, integral handles persistent offset, and derivative adds damping based on how quickly error changes. I clamp the command because real actuators have limits, and I guard dt because bad dt can explode the derivative term.
```

When explaining a noisy sensor:

```text
I would filter the measurement, but I would be careful because filtering adds delay. In a feedback loop, too much delay can destabilize the controller, so I would start with light filtering and validate the response on logs or hardware.
```

When explaining sim-to-real:

```text
A controller can work in simulation and fail on hardware because the real system has latency, backlash, friction, noisy sensors, saturation, calibration errors, and sometimes wrong signs or units. I would test with small commands first and compare logged target, measured state, and command.
```
