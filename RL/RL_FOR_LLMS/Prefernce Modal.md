# **Note:** AI was used to assist with research/planning. Read Section 8 additional notes to find out more.

# **Preference Model \- RL Environment Design**

## **1\. Environment Description**

The environment I'd build is this: give the LLM a broken or underperforming RL training pipeline and ask it to debug, fix, and optimize it until the trained agent hits a target reward threshold on a task.

Because I love embodied AI and RL in robotics, in this example, the LLM would get a Python project with a PPO training script, an environment wrapper, a reward function, and a config file. The bug may be that the advantage calculation is wrong, or the clipping parameter is set to something absurd, or the reward function has a sign error, or there's a shape mismatch in the observation preprocessing that silently broadcasts instead of crashing. The LLM has to find the bugs, fix them, tune the hyperparameters, and get the agent to a target score on a Gym control task (e.g., Walker2d-v4 \>= 3000).

Current LLMs can answer trivia about RL (what's the Bellman equation, what does PPO clip), but suck at multi-step reasoning like actually reading through a training loop, understanding why the reward curve is flat or the environment wrapper or the hyperparameters \-- that's multi-step reasoning over code plus domain knowledge, which is genuinely hard. Meta's MLGym benchmark found the same thing: frontier models can tune hyperparameters but can't diagnose bugs that may be affected by mult-reasnong. 

It's also a task that real ML engineers do constantly. I've spent more time staring at flat reward curves and wondering what's wrong than I've spent on any other part of RL work. It's an authentic problem.

## **2\. Tools, Packages, and Data**

Beyond the command line tool for reading, writing, and running files:

* Python 3.10+ with PyTorch, Gymnasium, NumPy, and a minimal PPO implementation in the codebase  
* MuJoCo (Walker2d dependency) pre-installed  
* TensorBoard or simple logging so the LLM can inspect training curves after running training  
* A pre-seeded random state so training is deterministic  
* No GPU needed, no internet access. Everything the LLM needs is in the venv.

## **3\. Prompt**

| You are an ML engineer. You have been given an RL project in ./HumanoidTraining that trains a PPO agent on Walker2d-v4 (MuJoCo) using PyTorch.The code has bugs and/or suboptimal configurations that prevent the agent from reaching good performance. Your job is to:1\. Read and understand the codebase in /project/ (train.py, [ppo.py](http://ppo.py), config.yaml, env\_wrapper.py)2\. Identify bugs, errors, or misconfigurations3\. Fix the code so that running \`python /project/train.py\` trains an agent that achieves an average evaluation reward \>= 3000 over 100 episodes4\. You may modify any file in /project/. Do not modify files outside /project/.5\. When done, ensure \`python /project/train.py\` runs without errors and saves a model at /project/checkpoints/final\_model.ptThe agent will be evaluated by loading final\_model.pt and running it for 100 episodes on Walker2d-v4. The average reward determines your score. |
| :---- |

## **4\. Judge Description**

The judge does the following:

(1) Check if the final model file exists. If not, score 0\.

(2) Check that [train.py](http://train.py) runs without errors. If it crashes, score 0\.

(3) Load the saved model. If loading fails (wrong format, missing architecture), score 0\.

(4) Run the model for 100 episodes on Walker2d-v4 with seed=42, recording total reward per episode.

(5) Compute average reward. Scoring: avg \< 0 \= 0.0 | 0 to 3000 \= avg/3000 (linear partial credit) | \>= 3000 \= 1.0.

The LLM fails if the model file doesn't exist, training crashes, the model can't load, or the agent walks poorly. The LLM succeeds if it finds the bugs, fixes them, tunes training to converge, and the walker hits the target score.

## **5\. Reward Hacking and Reward Denial**

The main risk is the LLM skipping debugging entirely and writing a hardcoded policy, downloading pre-trained weights, or tampering with the evaluator. My research from reading the METR article said exactly this with o3 that it found the answer key inside grading code, monkey-patched evaluators to always return True.

The judge runs in a separate sandbox the LLM cannot access or read. No internet access prevents downloading pre-trained weights. The evaluation seed is not in the prompt, so hardcoding actions for specific trajectories won't work. The judge checks that training actually runs (not just that a model file appears).

**Denial risks:** RL training has a lot of variance. The fix is fixed evaluation seed plus continuous scoring with partial credit. An agent averaging 2500 still gets a score 0.83, which is a meaningful signal rather than a hard cliff at 3000\. The bugs are also chosen so a correct fix converges well within the time budget.

## **6\. Why This Environment**

I come from robotics RL so I have experience implementing PPO for continuous control and worked in Gym environments. I picked this environment because debugging broken training loops is what I've done the most of, and it's a problem where I can tell the difference between a good judge and a bad one from experience.

## **7\. GitHub**

I built this to get hands on with RLVR before writing the take-home. The task was to design an RL environment for LLM training. I wanted to actually run a pipeline before proposing one.

[https://github.com/hassaan141/LearningLLMRL](https://github.com/hassaan141/LearningLLMRL)

## **8\. Additional Notes**

Since I come from a Robotics AI background I am more familiar with PPO and what RLHF is with LLMs but had to read up to understand RLVR. (Check resources section under). Even though the task was just to plan, to get familiar and for my own understanding, I built a toy GRPO training loop (using Qwen2.5-0.5B and TRL) to quickly understand a simple pipeline before writing this proposal. It uses string-matching rewards on ML trivia questions (much simpler than what I'm proposing here) but it helped me understand how the training loop, reward function, and policy update fit together. The gap between that toy setup and a real environment is mostly in the judge. String matching on trivia is easy to verify but not very hard. Real tasks which evaluate execution where it gets hard and where reward hacking becomes a harder issue to solve.

**9\. Resources**

"Training language models to follow instructions with human feedback" (Ouyang et al., 2022 — the InstructGPT paper)

"DeepSeek-R1: Incentivizing Reasoning Capability in LLMs via Reinforcement Learning" (DeepSeek-AI, Jan 2025 — [arxiv.org/abs/2501.12948](http://arxiv.org/abs/2501.12948)

"Reward Hacking in Reinforcement Learning" (Lilian Weng) [lilianweng.github.io/posts/2024-11-28-reward-hacking/](http://lilianweng.github.io/posts/2024-11-28-reward-hacking/)

"Recent Frontier Models Are Reward Hacking" (METR, June 2025 — [metr.org/blog/2025-06-05-recent-reward-hacking/](http://metr.org/blog/2025-06-05-recent-reward-hacking/)

