# Preference Model Job Description Walkthrough

## The role in one sentence

Preference Model wants interns who can turn realistic ML/research/software tasks into reproducible RL environments for LLMs: a prompt, a sandbox, tools/files, a judge, reward design, anti-cheating defenses, experiments, and production-quality Python.

This is closer to "build the benchmark/training environment frontier labs can use for RL" than "train a chatbot with RLHF from scratch."

## What the job description is really testing

### "Build RL training environments for large language models"

An LLM environment is not only a text prompt. For this company, think:

```text
Docker container + repo/files/data + task prompt + allowed tools + hidden judge + scoring function
```

The LLM acts inside that environment. It may read files, edit code, run tests, train a model, debug logs, or produce an artifact. The judge converts the result into a score/reward.

Your robotics translation:

```text
Gym/MuJoCo environment      -> Docker/code environment
robot policy                -> LLM policy
continuous action           -> next token / tool action / code edit
physics reward              -> programmatic judge
episode rollout             -> one attempted task solution
reward hacking in sim       -> LLM cheating the judge/tests/files
```

### "Real-world complexity"

They do not want toy string matching if the real task is engineering. They want tasks that look like actual work:

- debug a broken training loop
- improve a codebase without breaking tests
- reproduce a paper result
- inspect logs and identify a system issue
- tune an ML experiment under constraints
- write a robust evaluator for a messy output

The hard part is not only the model update algorithm. The hard part is environment quality.

### "Robust reward functions"

A good reward function should be:

- verifiable: the judge can programmatically decide quality
- hard to hack: the model cannot read, edit, or bypass the evaluator
- graded if possible: partial credit is better than only pass/fail
- stable: repeated runs should not swing wildly due to randomness
- aligned with the intended skill: scoring should reward solving the task, not exploiting a shortcut

For your Walker2d/PPO debugging idea, the judge is the most important piece. The interview discussion should focus on hidden evaluation, deterministic seeds, artifact checks, runtime budget, and how to prevent hardcoded policies or evaluator tampering.

## What each qualification means

### Strong Python

They likely mean normal software engineering Python:

- clean modules, not only notebooks
- CLI scripts and config files
- subprocesses, file IO, logging, exceptions
- pytest-style tests
- reproducible execution
- Docker-friendly code

Good interview line:

> "I would treat the environment like production infrastructure: deterministic setup, clear entrypoint, judge isolated from the agent, logs for debugging, and tests for both the task and the evaluator."

### Familiarity with how LLMs work

You should be able to explain the full stack:

```text
tokens -> embeddings -> transformer blocks -> logits -> sampled tokens
pretraining -> instruction tuning -> preference tuning / RL
inference -> sampling, context window, KV cache
```

You do not need to derive every equation from memory, but you should understand what happens well enough to reason about model failure modes.

### Transformer internals

High-value topics from `training-gpt`:

- tokenization: text becomes token IDs, not words
- embeddings: learned vectors for token identity
- positional encoding/RoPE: injects order into attention
- attention: Q/K/V, causal mask, softmax weights, O(n^2) context cost
- transformer block: attention + MLP + residual + normalization
- logits: raw scores over vocabulary for the next token
- training: cross-entropy next-token prediction, backprop, AdamW
- inference: autoregressive generation, temperature/top-p/top-k, KV cache

### Docker/reproducible environments

This is central. Their product depends on tasks that can run again and again for different models. Be ready to discuss:

- pinned dependencies
- offline operation
- fixed seeds
- timeouts
- resource limits
- hidden tests/judges
- separating agent workspace from evaluator workspace
- logging enough to debug failures

### Translate papers into tasks

This means taking a paper/concept and asking:

```text
What skill does this paper demonstrate?
Can an LLM attempt that skill in a sandbox?
What files/data/tools should it receive?
What output should it produce?
How do we judge it automatically?
How do we prevent shortcut solutions?
How do we calibrate difficulty?
```

Your proposed PPO-debugging environment is a good answer because it maps an ML skill into an execution-based task.

## Training GPT: interview-oriented reading path

Read `training-gpt` for the job in this order:

1. `chapters/00_overview.md`
   - Goal: say what a GPT is in one minute.
   - Key phrase: "A decoder-only transformer trained to predict the next token."

2. `chapters/02_tokenization.md`
   - Goal: explain why tokens are subword pieces and why exact string rewards can be brittle.
   - Interview tie-in: reward functions often evaluate text artifacts, but the model acts through tokens.

3. `chapters/03_embeddings.md`
   - Goal: explain how token IDs become vectors the model can compute with.

4. `chapters/05_attention.md`
   - Goal: explain Q/K/V and causal masking.
   - Interview tie-in: model limitations include context length, retrieval over long files, and attention cost.

5. `chapters/06_transformer_block.md`
   - Goal: explain residuals, norm, MLP, and why modern models use pre-norm/RMSNorm/SwiGLU.

6. `chapters/07_gpt_model.md`
   - Goal: connect all pieces into logits.
   - Key phrase: "For each position, the model outputs logits over the vocabulary."

7. `chapters/08_training.md`
   - Goal: explain next-token training, loss, gradients, AdamW, mixed precision.

8. `chapters/09_inference.md`
   - Goal: explain generation as repeated next-token sampling.
   - Interview tie-in: during RL, the model samples completions/actions, the judge scores them, and the update makes good trajectories more likely.

9. `fine-tuning/01_what_is_finetuning.md`, `02_lora_explained.md`, `06_dpo_explained.md`
   - Goal: distinguish pretraining, SFT, LoRA/QLoRA, DPO, RLHF, and RLVR.

## The complete LLM training pipeline

Use this mental model:

```text
1. Pretraining
   Train on internet/books/code to predict next token.
   Learns language, facts, code patterns, reasoning priors.

2. Instruction tuning / SFT
   Train on prompt-response examples.
   Learns to follow user instructions and produce chat-style answers.

3. Preference optimization
   Use chosen/rejected data or reward signals.
   DPO: directly train on preference pairs.
   RLHF: train/use a reward model, then optimize the policy.
   RLVR: use a verifiable judge such as tests, math checker, execution result.

4. Inference
   The trained model generates token by token using sampling/decoding.
```

Preference Model lives mostly around step 3, especially RLVR-style environments where the reward can be checked programmatically.

## RLHF, RLVR, GRPO, PPO: clean definitions

### RLHF

Human preference data trains a reward model or preference objective. Useful when quality is subjective: helpfulness, tone, harmlessness, writing quality.

Risk: the policy may learn to exploit weaknesses in the reward model.

### RLVR

The reward comes from verification: tests pass, answer is correct, program runs, proof checks, metric improves.

Useful when correctness can be checked. This is the most relevant frame for Preference Model.

### PPO

An RL algorithm that updates the policy while limiting how far it moves from the old policy. In LLM RLHF, PPO commonly uses a reward model, a value/critic head, and a KL penalty against a reference model.

Important nuance: PPO itself does not require a learned reward model. In RLVR, the reward can be rule-based.

### GRPO

An RL method that compares multiple completions for the same prompt and uses group-relative scores instead of a learned critic/value model. This reduces memory/compute compared with critic-based methods.

Important nuance: the update uses token log-probabilities of generated completions, often with a reference/KL term in real training setups.

## Corrections/tighten-ups for your existing notes

- Say "Preference Model builds RL environments and judges used in training runs" instead of "they don't train LLMs." The job post says they deliver work into production training runs, so avoid overclaiming their boundary.
- "DPO replaced RLHF for most teams" is too broad. Safer: "DPO is widely used because it is simpler and more stable for offline preference data; PPO-style RLHF/RLVR is still used when online sampling and verifiable rewards matter."
- "PPO needs 4 models in memory" is a useful intuition but not universal. Safer: "LLM PPO often involves a policy, reference model, reward model or rule reward, and value/critic head."
- Avoid saying ChatGPT/Claude reason step-by-step only because RL reinforced it. Safer: "RL and preference training can reinforce reasoning-like traces when they improve judged outcomes, especially on hard verifiable tasks."
- Your take-home environment is strongest when you emphasize judge design, not only PPO knowledge.

## Interview answer: why you fit

> My background is robotics RL, so I already think in terms of environments, reward functions, episode rollouts, variance, and reward hacking. What clicked for me is that Preference Model is applying the same environment-design problem to LLMs. The agent is now a language model acting through tokens and tools, and the environment is a Dockerized code/research task with a hidden judge. I built a toy GRPO loop to understand the LLM side, and my proposed PPO-debugging task is meant to connect that with the kind of real ML work where a verifiable judge can actually measure progress.

## Questions to ask them

- What makes an RL environment good enough to ship into an actual training run?
- How do you evaluate whether an environment is too easy, too hard, or hackable?
- What kinds of tasks have been most useful so far: coding, ML experiments, systems debugging, paper reproduction?
- How much of the intern role is environment design versus infrastructure/debugging?
- What does productionizing an environment usually involve?

