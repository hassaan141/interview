## 100 robotics SWE questions

## Foundations: robotics, control, and real systems
# What is the difference between an open-loop and closed-loop control system?

# What is feedback control?

# What is a PID controller, and what does each term do?

# What happens if proportional gain is too high?

# What happens if integral gain is too high?

# What happens if derivative gain is too high or too noisy?

# What is steady-state error?

# What is overshoot?

# What is settling time?

# What is control loop bandwidth?

# What is the difference between stability and performance?

# What is phase lag and why can it destabilize a system?

# What is latency in a control loop?

# Why is jitter dangerous in a real-time control system?

# What is feedforward control?

# When would you combine feedforward and feedback?

# What is actuator saturation?

# What is integrator windup?

# How do you prevent or mitigate integrator windup?

# What is hysteresis?


## Motion, kinematics, geometry
# What is the difference between position, velocity, and acceleration control?

# What is jerk and why might it matter for a precision robot?

# What is the difference between forward kinematics and inverse kinematics?

# What is a Jacobian in robotics?

# What does it mean for a manipulator to hit a singularity?

# Why are singularities a problem in control or planning?

# What is workspace?

# What is the difference between joint space and Cartesian space control?

# When would joint space planning be preferable to Cartesian planning?

# What is homogeneous transformation?

# What is the difference between a rotation matrix and a quaternion?

# Why are quaternions often preferred over Euler angles?

# What is gimbal lock?

# What is a frame transform?

# What is calibration between coordinate frames?

# What is hand-eye calibration?

# What is the difference between absolute accuracy and repeatability?

# Why can a robot be highly repeatable but poorly accurate?

# What is backlash?

# What is compliance, and when is it helpful versus harmful?


## Planning and trajectory generation
# What is the difference between path planning and trajectory planning?

# What makes a trajectory dynamically feasible?

# What is motion planning?

# What is the difference between RRT and A* at a high level?

# What is collision checking?

# What is trajectory interpolation?

# Why might linear interpolation in joint space be unsafe or undesirable?

# What are trapezoidal and S-curve motion profiles?

# Why is jerk-limited motion often used in precision systems?

# What constraints matter in robot trajectory generation?

# What is time synchronization between commanded trajectory and actual motion?

# What is waypoint following?

# What is trajectory replanning?

# What is the tradeoff between optimality and real-time feasibility in planning?

# What is a safety envelope in motion planning?

# What is soft limit versus hard limit?

# What is an emergency stop supposed to do?

# What is the difference between stopping fast and stopping safely?

# How would you design motion software so failures default to a safe state?

# What would you log during a failed motion event?


## Sensors, perception, and estimation
# What is sensor fusion?

# What is the difference between accuracy, precision, resolution, and noise?

# What is bias in a sensor?

# What is drift?

# What is quantization noise?

# What is aliasing?

# Why is sampling frequency important?

# What does the Nyquist criterion say?

# What is filtering, and when might a low-pass filter help?

# Why can too much filtering hurt control performance?

# What is an encoder, and what does it measure?

# What is the difference between incremental and absolute encoders?

# What is IMU sensor fusion trying to estimate?

# What is the purpose of a Kalman filter at a high level?

# What assumptions does a Kalman filter make?

# What is the difference between localization and tracking?

# What is camera intrinsic calibration?

# What is camera extrinsic calibration?

# What is stereo vision used for?

# What is the difference between rolling shutter and global shutter?

# Why might rolling shutter be problematic on a fast-moving robot?

# What does latency in a vision pipeline do to closed-loop control?

# What is perception-to-control handoff, and where do bugs often happen?

# How do you validate that a vision-based measurement is trustworthy enough for surgery-related motion?

# What would you do if a perception system intermittently outputs clearly wrong poses?


## Embedded, Linux, and systems integration
# What is a real-time operating constraint, even if you are running on Linux?

# What is the difference between soft real-time and hard real-time?

# What Linux debugging tools would you use for a robotics application?

# What is the difference between a crash, a hang, and a livelock?

# What is a watchdog timer?

# Why do watchdogs matter in robot software?

# What is priority inversion?

# How can priority inversion affect a robotic control loop?

# What is a CAN bus and why is it common in robotics?

# What is SPI and what factors determine its usable clock rate?

# What is the difference between UART, I2C, SPI, and CAN in practical engineering terms?

# What is clock drift and why does it matter across multiple devices?

# How would you synchronize timestamps between sensors and motor controllers?

# How would you debug a hardware-software integration bug that only appears on the actual robot?

# How would you design software for a surgical robot so that it remains operable, diagnosable, and safe when one subsystem starts behaving unexpectedly?