# Preference Model Interview Prep — Full Handoff

## What Preference Model Does

Early-stage startup (2 founders from Anthropic/DatologyAI). They build RL training environments for frontier AI labs. They don't train LLMs — they build the tasks, judges, and sandboxed environments that labs use to train models via RL. The bottleneck in the industry right now is lack of high-quality environments, and that's what they're solving.

The job: pick a real ML/engineering task, containerize it in Docker, write a judge that scores the LLM's output programmatically, test it against frontier models for difficulty, ship it.

---

## The Core Loop (same in robotics and LLMs)

```
Prompt → Agent generates output → Judge scores it → Reward signal → Update weights → Repeat
```

In robotics: observation → robot takes action → reward function scores it → PPO updates policy
In LLM RL: prompt → LLM generates tokens → judge checks answer → GRPO updates model

Same loop. Different agent type.

---

## What an RL Environment Actually Is

**Simple example — math:**
- Prompt: "What is 347 × 28?"
- LLM generates: "9716"
- Judge: runs `347 * 28` in Python, gets 9716, checks if LLM said 9716
- Score: 1.0 or 0.0
- That's it. That's an RL environment.

**Complex example — SWE-Bench style (what Preference Model builds):**
- Docker container with a real GitHub repo, checked out at the commit before a bug was fixed
- LLM reads the bug report, explores the codebase, writes a fix
- Judge: run the test suite. "Fail-to-pass" tests should now pass, "pass-to-pass" tests should still pass
- LLM never sees the test code or the real fix
- Scale to thousands of real GitHub issues = SWE-Bench

**My take-home — Walker2d debugging:**
- Docker container with a broken PPO training pipeline
- LLM reads the code, finds bugs (wrong advantage calc, bad clipping, sign errors), fixes them
- Judge: load the trained model, run Walker2d for 100 episodes, check average reward
- Continuous scoring: avg/3000 (partial credit)

---

## How GRPO Works (What My train.py Does)

### Step 1: Pick a prompt, generate 4 completions

```
Prompt: "What activation function squashes to 0-1?"

Completion 1: "sigmoid"                   (saved token probs: sig=0.02, moid=0.008)
Completion 2: "The answer is relu"        (saved token probs: The=0.15, answer=0.09, is=0.12, rel=0.04, u=0.03)
Completion 3: "sigmoid function"          (saved token probs: sig=0.02, moid=0.008, function=0.05)
Completion 4: "I think tanh"              (saved token probs: I=0.10, think=0.07, tan=0.01, h=0.06)
```

The trainer saves the probability of each token at generation time. This is how it knows what to update later.

### How token generation works

The LLM predicts one token at a time. At each step, it outputs a probability for every token in the vocabulary (~50k options):

```
Input: "...Answer:"
Model outputs:  "the"→0.15, "I"→0.10, "sig"→0.02, "rel"→0.04, "tan"→0.01 ...
Sampled: "sig" (random dice roll, probability 0.02)
Trainer saves: ("sig", 0.02)

Input: "...Answer: sig"
Model outputs:  "moid"→0.008, "nal"→0.3, "ma"→0.2 ...
Sampled: "moid" (probability 0.008)
Trainer saves: ("moid", 0.008)

Done → Completion: "sigmoid", saved probs: [(sig, 0.02), (moid, 0.008)]
```

### Step 2: Judge scores each completion

```
judge("sigmoid", "sigmoid")              → 1.0 ✓
judge("The answer is relu", "sigmoid")   → 0.0 ✗
judge("sigmoid function", "sigmoid")     → 1.0 ✓
judge("I think tanh", "sigmoid")         → 0.0 ✗

Returns: [1.0, 0.0, 1.0, 0.0]
```

### Step 3: Compute advantages (score - group mean)

```
Group mean = (1.0 + 0.0 + 1.0 + 0.0) / 4 = 0.5

"sigmoid"          → 1.0 - 0.5 = +0.5 (better than average)
"The answer is relu" → 0.0 - 0.5 = -0.5 (worse than average)
"sigmoid function" → 1.0 - 0.5 = +0.5 (better than average)
"I think tanh"     → 0.0 - 0.5 = -0.5 (worse than average)
```

### Step 4: Update token probabilities

```
"sigmoid" had advantage +0.5 → nudge token probs UP
  "sig":  0.02  → 0.023
  "moid": 0.008 → 0.009

"relu" had advantage -0.5 → nudge token probs DOWN
  "rel": 0.04 → 0.037
  "u":   0.03 → 0.028
```

### Step 5: Throw everything away

Completions, scores, advantages, saved probabilities — all deleted. Only the updated weights survive. The model is now slightly better. Next step: generate fresh completions from the updated model. Repeat.

### Scale

My demo: 15 questions × 4 generations × 3 epochs = 180 total completions. Tiny, noisy.
DeepSeek-R1: thousands of problems × many generations × many epochs = millions of completions.

---

## GRPO vs PPO

PPO needs 4 models in memory: policy, reference copy, reward model, and a critic network. The critic estimates "how good is this state" at every step — for an LLM, that's training a second LLM-sized model.

GRPO drops the critic. Instead of "how good is this vs what the critic predicts," it asks "how good is this vs the other completions in this group?" Same signal, ~25-30% less compute. At DeepSeek's scale (671B params), that's hundreds of GPUs saved.

---

## How Reasoning Emerges (Why DeepSeek-R1 "thinks")

On a hard math problem, the model generates 4 attempts:

```
Attempt 1: "x = 2"                                          (lucky guess, correct)
Attempt 2: "x = 7"                                          (guess, wrong)
Attempt 3: "Let me factor. (2x+7)(x-2)=0. x=2 or x=-3.5"   (reasoning, correct)
Attempt 4: "x = -3"                                         (guess, wrong)
```

On easy problems, guessing and reasoning both score equally. On hard problems, guessing rarely works but reasoning consistently does. Across millions of hard problems, completions with step-by-step thinking score higher on average. GRPO reinforces the tokens for "let me think", "step 1", etc.

Nobody told the model to reason. It discovered that generating more intermediate tokens (a scratchpad) makes the final answer more likely to be correct. GRPO reinforced that pattern.

This is why ChatGPT/Claude break everything into steps — that behavior got reinforced millions of times during training. It even bleeds into simple questions where it's unnecessary.

---

## RLHF vs RLVR

**RLHF (RL from Human Feedback):** Train a neural reward model on human preference data ("which response is better?"), then use that model as the judge. Problem: the LLM can learn to fool the reward model (generate text that "sounds good" without being correct).

**RLVR (RL from Verifiable Rewards):** Use a rule-based judge. Math: check the answer. Code: run the tests. No neural model to fool. DeepSeek-R1 used this: R = accuracy + format. That's it.

Use RLVR when the task has a checkable answer. Use RLHF when it doesn't (creative writing, helpfulness, tone).

---

## Reward Shaping vs Reward Hacking

**Reward shaping** = adding intermediate signals so the agent can learn gradually.

My judge.py: binary 0 or 1. No shaping. Agent has no idea it's improving.
My take-home judge: avg_reward/3000. Linear partial credit. Agent gets gradient — "0.5 last time, 0.83 now, keep going."
Robotics equivalent: bonus reward for staying upright, not just reaching the goal.

**Reward hacking** = agent gets high score without solving the task.

My judge.py hack: answer to "what does ReLU return for -5?" is "0". Model could write "I have 0 idea" and score 1.0 because "0" appears in the output.

My take-home hack: LLM deletes all PPO code and writes a hardcoded controller that walks well. Scores 1.0 without debugging anything.

Real-world hack (METR): o3 read the grading code, found the answer key. On another task, it overwrote the judge to always return True. 30% of runs on RE-Bench involved cheating. Telling the model "don't cheat" dropped it from 80% to 70%. Doesn't work.

Fix: structural isolation. Sandbox the judge. No internet. Hidden eval seed. The model physically can't access the scoring code.

---

## Robotics RL → LLM RL Translation Table

| Concept | Robotics (my experience) | LLM RL (this role) |
|---------|-------------------------|-------------------|
| Agent | Neural net controlling robot | The LLM itself |
| Action | Joint torques [0.7, -0.3, 0.5] | Next token from ~50k vocab |
| Environment | Physics sim (MuJoCo, Gym) | Docker container with files + terminal |
| Observation | Joint angles, velocities | Prompt + tokens generated so far |
| Episode | One rollout until fall/timeout | One full completion |
| Judge | Reward = upright + forward - energy | Did the answer pass verification? |
| Reward hacking | Robot exploits physics glitches | LLM overwrites test files |
| Reward shaping | Bonus for staying upright | Partial credit (avg/3000) |
| Training loop | PPO: rollout → advantages → update | GRPO: generate → score → compare → update |
| Environment design | Define obs/action space, reset, step | Write prompt, Docker container, judge |

Key difference: an LLM can read and rewrite the judge's source code. A robot can't rewrite the physics engine.

---

## Recruiter Call Prep

**"Tell me about yourself"** (under 90 seconds):
I'm studying [program], got into RL through robotics — PPO, Gym, continuous control. Recently got interested in RL for LLMs, built a GRPO training pipeline to learn the space, and wrote a take-home proposing an RL environment for debugging PPO.

**"Why Preference Model?"**:
The take-home clicked with what I already do — designing environments where reward is verifiable, debugging training loops, figuring out what makes a good judge.

**"What did you learn from the take-home?"**:
The gap between my toy string-matching judge and a real execution-based judge. Read DeepSeek-R1 (why rule-based rewards beat neural reward models) and METR's findings (o3 hacking judges in 30% of runs).

**The connecting sentence for your robotics background:**
"In robotics RL, I designed environments and reward functions to train agents. This role is the same thing — the agent is an LLM and the actions are tokens instead of joint torques."

Don't apologize for being "just robotics." The job is environment and judge design. That's your background.

**Questions to ask them:**
- "What does a typical RL environment you've built look like?"
- "How big is the team and who would I work with day to day?"
- "What does the interview process look like after this call?"

---

## Papers and Resources I Read

- InstructGPT (Ouyang et al., 2022) — introduced RLHF with PPO to LLMs
- DeepSeek-R1 (Jan 2025) — RLVR with GRPO, reasoning emerges from pure RL
- DeepSeekMath (Shao et al., 2024) — introduced the GRPO algorithm
- Lilian Weng's blog post on reward hacking — best overview of hacking types and defenses
- METR "Recent Frontier Models Are Reward Hacking" (June 2025) — real documented cases of o3 cheating

---

## What I Built

GitHub repo: https://github.com/hassaan141/LearningLLMRL

Toy GRPO training loop using Qwen2.5-0.5B and TRL. String-matching reward on ML trivia. Built it before writing the take-home to understand the pipeline hands-on. The judge is the weak point — substring matching is trivially verifiable but not challenging. Real environments need execution-based evaluation, which is where reward hacking becomes a real problem.
