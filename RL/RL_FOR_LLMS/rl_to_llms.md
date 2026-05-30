Agent	Neural network controlling a robot
e.g. your Walker2d policy
The LLM itself
e.g. DeepSeek-V3, Qwen
Action	Joint torques, motor commands
continuous floats like [0.7, -0.3, 0.5]
Next token
sampled from ~50k vocabulary
Environment	Physics simulator (MuJoCo, Gym)
gravity, friction, collisions
Docker container with files, terminal
codebase, dataset, test suite
Observation	Joint angles, velocities, sensor data
float vector like [0.2, -1.4, 3.1, ...]
The prompt + all tokens generated so far
text context window
Episode	One rollout until robot falls or timeout
~1000 timesteps
One full completion from prompt to end
~100-1000 tokens
Reward function / judge	Reward = stay upright + move forward - energy
you designed this in Gym
Judge = did the answer pass verification?
run tests, check math, string match
Reward hacking	Robot exploits physics glitches
vibrating to gain speed, falling "stylishly"
LLM overwrites test files, hardcodes answers
monkey-patches the judge
Reward shaping	Add small rewards for sub-goals
bonus for staying upright, facing forward
R = accuracy + format penalty
DeepSeek penalizes bad formatting
Training loop	PPO: rollout → compute advantages → update
needs a critic network
GRPO: generate → score → compare group → update
no critic needed
Environment design	Define obs space, action space, reset, step
Gym API: reset(), step(), render()
Write prompt, build Docker container, write judge
this is literally the job at Preference Model
Difficulty tuning	Change terrain, gravity, body mass
curriculum learning
Pick harder tasks, add more subtle bugs
test against frontier models to calibrate
Generalization	Train on one terrain, test on another
does the walker handle slopes?
Train on seen problems, test on unseen
does the model reason on new tasks?
