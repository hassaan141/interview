# PyTorch RL Cheat Sheet

This is the running reinforcement learning cheat sheet for `Pytorch/pytorch-rl`.
After each lesson, append the mental model, key formulas, PyTorch patterns, and mistakes to avoid.

## Learning Roadmap
- Lesson 0: Gym basics, environments, action/observation spaces, wrappers, RL loop.
- Lesson 1: Vanilla Policy Gradient / REINFORCE.
- Lesson 2: Actor-Critic.
- Lesson 3: Advantage Actor-Critic / A2C.
- Lesson 4: Generalized Advantage Estimation / GAE.
- Lesson 5: Proximal Policy Optimization / PPO.

## RL Big Picture
- In supervised learning, you get `(x, y)` pairs and train against labels.
- In reinforcement learning, an agent interacts with an environment:
  1. observe state
  2. choose action
  3. receive reward
  4. observe next state
  5. repeat until episode ends
- The goal is not to predict a label. The goal is to learn behavior that maximizes long-term reward.

Core loop:

```python
state, info = env.reset(seed=1234)
terminated = False
truncated = False
episode_reward = 0

while not (terminated or truncated):
    action = agent.act(state)
    next_state, reward, terminated, truncated, info = env.step(action)
    done = terminated or truncated

    agent.learn(state, action, reward, next_state, terminated, truncated)

    state = next_state
    episode_reward += reward
```

## Core Vocabulary
- Agent: the learner/decision maker.
- Environment: the world the agent interacts with.
- State / observation: what the agent sees.
- Action: what the agent does.
- Reward: scalar feedback from the environment.
- Episode: one full rollout from reset until `terminated=True` or `truncated=True`.
- Policy: mapping from state to action.
- Return: total future reward, usually discounted.
- Value function: expected return from a state or state-action pair.

## Lesson 0: Introduction to Gym

### 0) Modern Gymnasium API
The old tutorial used OpenAI Gym. Current code should usually use Gymnasium:

```python
import gymnasium as gym
```

Main migration changes:

```python
# old Gym
state = env.reset()
state, reward, done, info = env.step(action)
env.seed(1234)

# modern Gymnasium
state, info = env.reset(seed=1234)
next_state, reward, terminated, truncated, info = env.step(action)
done = terminated or truncated
```

Why `terminated` and `truncated` are split:
- `terminated`: the actual task ended, like CartPole falling.
- `truncated`: the episode stopped because of a limit, often max steps.
- For many simple loops, `done = terminated or truncated` is fine.
- For bootstrapping algorithms, the distinction matters.

Atari naming changed too:

```python
# old
gym.make("Pong-v0")
gym.make("FreewayNoFrameskip-v4")

# modern
gym.make("ALE/Pong-v5")
gym.make("ALE/Freeway-v5", frameskip=1, repeat_action_probability=0.0)
```

### 1) What Gymnasium gives you
Gymnasium provides standard RL environments with a shared API:

```python
import gymnasium as gym

env = gym.make("CartPole-v1")
state, info = env.reset(seed=1234)
next_state, reward, terminated, truncated, info = env.step(action)
done = terminated or truncated
```

Most important methods/properties:

```python
env.reset(seed=1234) # start a new seeded episode
env.step(action)     # apply action, get next transition
env.action_space    # valid actions
env.observation_space  # shape/range/type of observations
env.spec.max_episode_steps
env.spec.reward_threshold
```

### 2) Observation spaces
The observation space tells you what shape/type the state has.

Common space types:

```python
gym.spaces.Discrete(n)  # integer from 0 to n-1
gym.spaces.Box(...)     # continuous tensor/array range
```

CartPole observations are continuous:

```text
[cart_position, cart_velocity, pole_angle, pole_angular_velocity]
```

So even though CartPole has simple left/right actions, the input state is a float vector.

### 3) Action spaces
The action space tells you what actions are legal.

CartPole:

```text
Discrete(2)
0 = push cart left
1 = push cart right
```

Sampling a random action:

```python
action = env.action_space.sample()
```

This is useful for smoke testing an environment before adding a real policy.

### 4) Discrete vs continuous control
Discrete actions:

```python
env = gym.make("CartPole-v1")
action = 0  # one of a fixed number of choices
```

Continuous actions:

```python
env = gym.make("Pendulum-v1")
action = env.action_space.sample()  # usually a float vector
```

Why it matters:
- Discrete control often uses categorical policies or Q-values.
- Continuous control often uses Gaussian policies or direct action outputs.

### 5) Atari-style observations
Atari environments usually return image observations:

```text
height x width x channels
```

That changes the model choice:
- vector state like CartPole -> MLP is enough
- image state like Atari -> CNN is usually needed

Atari action meanings can be inspected:

```python
env.unwrapped.get_action_meanings()
```

### 6) Wrappers
Wrappers modify observations, rewards, or actions without rewriting the environment.

Reward wrapper example:

```python
class ClipRewardEnv(gym.RewardWrapper):
    def __init__(self, env):
        super().__init__(env)

    def reward(self, reward):
        return np.sign(reward)
```

Observation wrapper example:

```python
class ScaledFloatFrame(gym.ObservationWrapper):
    def __init__(self, env):
        super().__init__(env)
        self.observation_space = gym.spaces.Box(
            low=0.0,
            high=1.0,
            shape=env.observation_space.shape,
            dtype=np.float32,
        )

    def observation(self, observation):
        return np.asarray(observation, dtype=np.float32) / 255.0
```

Mental model:
- Reward wrappers change reward signals.
- Observation wrappers change state preprocessing.
- Action wrappers can change how actions are represented.

### 7) One episode with random actions
This is the basic RL data collection loop:

```python
env = gym.make("CartPole-v1")

state, info = env.reset(seed=1234)
terminated = False
truncated = False
episode_reward = 0

while not (terminated or truncated):
    action = env.action_space.sample()
    state, reward, terminated, truncated, info = env.step(action)
    episode_reward += reward

print(episode_reward)
```

### 8) RL loop vs normal PyTorch training loop
Normal supervised learning:

```python
prediction = model(x)
loss = loss_fn(prediction, y)
loss.backward()
optimizer.step()
```

RL learning:

```python
action = policy(state)
next_state, reward, terminated, truncated, info = env.step(action)
done = terminated or truncated
loss = rl_objective(...)
loss.backward()
optimizer.step()
```

The big difference: the data comes from the agent's own actions.

### 9) Lesson 0 mistakes to avoid
- Confusing `state` with a supervised-learning label. State is input, reward is feedback.
- Forgetting that `terminated=True` or `truncated=True` means the episode is over and you should call `reset()`.
- Assuming all environments return vectors. Atari returns images.
- Assuming all actions are discrete. Some environments use continuous action vectors.
- Training before smoke testing with random actions.
- Ignoring observation/action space shapes before designing the network.

### 10) 10-second memory
- `reset()` starts an episode.
- `step(action)` advances the world.
- `state` is what the agent sees.
- `action_space` tells what the agent can do.
- `observation_space` tells what the model input looks like.
- Reward is feedback, not a label.
- RL data is created by interaction.

## Appendix: RL Symbols
- `s_t`: state at time `t`
- `a_t`: action at time `t`
- `r_t`: reward after taking action
- `s_{t+1}`: next state
- `gamma`: discount factor for future rewards
- `G_t`: return from time `t`
- `pi(a|s)`: policy, probability of action `a` given state `s`
- `V(s)`: expected return from state `s`
- `Q(s, a)`: expected return from taking action `a` in state `s`

## Lesson 1: Vanilla Policy Gradient / REINFORCE

### 1) Big idea
REINFORCE directly trains a policy.

Instead of learning:

```text
state -> value of each action
```

it learns:

```text
state -> probability distribution over actions
```

For CartPole:

```text
input:  4 state values
output: 2 action logits
```

The policy samples actions during training so it can explore.

### 2) Policy network
CartPole uses a small MLP:

```python
class PolicyNetwork(nn.Module):
    def __init__(self, input_dim, hidden_dim, output_dim):
        super().__init__()
        self.fc1 = nn.Linear(input_dim, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, output_dim)

    def forward(self, x):
        x = F.relu(self.fc1(x))
        logits = self.fc2(x)
        return logits
```

Use logits directly:

```python
distribution = Categorical(logits=logits)
action = distribution.sample()
log_prob = distribution.log_prob(action)
```

This is cleaner than manually applying softmax first.

### 3) Why log probabilities matter
The sampled action is not differentiable by itself.

But the log probability of the sampled action is differentiable with respect to the policy network parameters.

So REINFORCE stores:

```python
log_prob_action = distribution.log_prob(action)
```

Later, it uses that to adjust the policy.

### 4) Return
The return is discounted future reward:

```text
G_t = r_t + gamma*r_{t+1} + gamma^2*r_{t+2} + ...
```

In code:

```python
returns = []
running_return = 0

for reward in reversed(rewards):
    running_return = reward + gamma * running_return
    returns.insert(0, running_return)
```

For CartPole, longer episodes mean larger returns because the agent gets reward for staying alive.

### 5) REINFORCE loss
The core loss:

```python
loss = -(returns * log_probs).sum()
```

Mental model:
- high return + chosen action -> make that action more likely
- low return + chosen action -> make that action less likely

The negative sign is there because PyTorch optimizers minimize loss, but RL wants to maximize reward.

### 6) Training loop
One REINFORCE update:

```python
state, info = env.reset(seed=seed)

while not (terminated or truncated):
    action, log_prob = select_action(policy, state)
    next_state, reward, terminated, truncated, info = env.step(action)

    log_probs.append(log_prob)
    rewards.append(reward)
    state = next_state

returns = calculate_returns(rewards, gamma)
loss = update_policy(returns, log_probs, optimizer)
```

REINFORCE waits until the episode ends before updating.

### 7) Train vs evaluate
Training:

```python
action = distribution.sample()
```

Evaluation:

```python
action = torch.argmax(logits, dim=-1)
```

Training samples actions to explore.

Evaluation usually picks the most likely action to measure what the policy has learned.

### 8) Common mistakes
- Using old Gym API: `state = env.reset()` and `state, reward, done, info = env.step(action)`.
- Forgetting Gymnasium returns `terminated` and `truncated`.
- Applying `softmax` and then using `Categorical(probs=...)` when `Categorical(logits=...)` is simpler.
- Forgetting to store `log_prob` for each sampled action.
- Updating every step instead of after the full episode for vanilla REINFORCE.
- Expecting policy loss to behave like supervised learning loss. Reward matters more.

### 9) 10-second memory
- Policy = network that outputs action logits.
- `Categorical` = turns logits into an action distribution.
- `sample()` = choose action during training.
- `log_prob()` = differentiable record of the chosen action.
- Return = discounted future reward.
- REINFORCE loss = `-(return * log_prob).sum()`.
