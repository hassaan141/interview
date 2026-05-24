# Preference Model — Initial Assessment Submission

## Environment Overview

**Title:** *Debug a non-converging supervised fine-tune of Qwen2-0.5B on a structured-output task.*

The environment hands the LLM agent a Python repository that fine-tunes `Qwen2-0.5B-Instruct` on a JSON-extraction task: given a natural-language product description, output a JSON object with a fixed schema (`{name, brand, price, category}`). The training script runs end-to-end without errors, but the fine-tuned model fails to learn the format — held-out exact-match accuracy stays near 0%. The LLM agent must diagnose and fix the underlying bugs, retrain, and produce a checkpoint that scores above a threshold on a held-out evaluation set the judge owns.

**What makes this environment interesting:**

- **Silent failures.** None of the planted bugs raise exceptions. Loss decreases monotonically during training; the model just learns the wrong thing. The LLM has to read the training loop while holding the math in its head — exactly the skill that distinguishes engineers who *understand* LLM training from those who only know the `transformers` API.
- **Realistic.** Loss-masking and tokenization bugs are the single most common silent failure mode in supervised fine-tuning. This is a real day-job task for an AI/ML engineer.
- **End-to-end verifiable.** The judge runs the actual training pipeline and evaluates the resulting model on examples the LLM never sees. There is no proxy metric to game.
- **Bounded difficulty.** A correctly-implemented fine-tune of Qwen2-0.5B on this task hits >85% exact-match with comfortable margin, so the threshold is achievable but only by actually fixing the bugs.

---

## Tools, Packages, and Data

**Tools available to the LLM agent:**
- Bash shell with read/write/execute on `/workspace/`.
- Python 3.10+ interpreter.
- Single GPU (A10 or equivalent) with a 30-minute training budget enforced by the judge.
- **No internet access** (prevents pulling pretrained task-specific checkpoints).

**Preinstalled packages:**
- `torch` (2.x)
- `transformers`, `datasets`, `accelerate`, `peft`
- `numpy`
- `tensorboard` (optional logging)

**Preinstalled model weights (offline cache):**
- `Qwen/Qwen2-0.5B-Instruct` only. Other base models are not available, preventing the LLM from sidestepping the task by swapping in a stronger pretrained model.

**Repository contents in `/workspace/`:**
- `train.py` — entry point: training loop (broken).
- `data.py` — dataset construction and tokenization (broken).
- `model_utils.py` — label masking / loss helpers (broken).
- `train_data.jsonl` — 2,000 training examples `{description, target_json}`.
- `eval.jsonl` — 100 examples the LLM can use for its own sanity checks.
- `requirements.txt`
- `README.md` — restates the task, threshold, and budget.

**Files the LLM cannot modify** (enforced by SHA-256 hash check in the judge):
- `eval.py` — defines the canonical inference and scoring interface.
- `judge.py` — orchestrates scoring.
- `train_data.jsonl`, `eval.jsonl` — data integrity.

**Held out from the LLM entirely:** the judge owns a separate 200-example evaluation set drawn from the same distribution but never written to disk inside `/workspace/`. The LLM cannot train on it.

---

## Prompt

> You are an ML engineer debugging a non-converging supervised fine-tune.
>
> The repository at `/workspace/` contains a script that fine-tunes `Qwen2-0.5B-Instruct` on a JSON-extraction task. Given a natural-language product description, the model should output a JSON object of the form:
>
> ```json
> {"name": "...", "brand": "...", "price": "...", "category": "..."}
> ```
>
> The script runs without errors, but the fine-tuned model achieves less than 5% exact-match accuracy on the local eval set at `/workspace/eval.jsonl` — it does not learn the output format.
>
> Your task is to modify the files in `/workspace/` so that running `python /workspace/train.py` produces a checkpoint at `/workspace/checkpoints/final/` that achieves a mean **exact-match accuracy of at least 80%** on the held-out evaluation set used by the judge.
>
> Constraints:
> - You may modify any file in `/workspace/` **except** `eval.py`, `judge.py`, `train_data.jsonl`, and `eval.jsonl`.
> - Training wallclock must not exceed 30 minutes on the provided GPU.
> - The checkpoint must be loadable via `transformers.AutoModelForCausalLM.from_pretrained("/workspace/checkpoints/final/")`.
> - You must fine-tune from `Qwen/Qwen2-0.5B-Instruct`. Do not use any other base model.
> - Inference at evaluation time is performed by `eval.py` using greedy decoding. Do not attempt to change inference behavior — fix the training.
>
> When you believe the implementation is fixed, run `python /workspace/train.py` to completion. The judge will then load your checkpoint and report a score.

---

## Judge

The judge executes the following pipeline:

1. **Tampering check.** SHA-256 hash `eval.py`, `judge.py`, `train_data.jsonl`, and `eval.jsonl` against stored originals. If any file is modified, return score 0.
2. **Base-model check.** Inspect the checkpoint's `config.json`; verify the architecture matches `Qwen2-0.5B`. Return 0 on mismatch.
3. **Training execution.** Run `python /workspace/train.py` with a 30-minute wallclock timeout and a fixed training seed. If the script raises an exception or times out, return score 0.
4. **Checkpoint check.** Verify `/workspace/checkpoints/final/` exists and loads via `AutoModelForCausalLM.from_pretrained()`. If not, return 0.
5. **Held-out evaluation.** Load the judge's private 200-example evaluation set. For each example, format the prompt using the canonical chat template (defined in `eval.py`), generate greedily with max 128 new tokens, parse the model output, and compare to the gold JSON via exact match after key-sorted JSON normalization.
6. **Score.**
   - **Binary variant:** pass if accuracy $\geq 0.80$, else fail.
   - **Continuous variant:** $\text{score} = \mathrm{clip}\!\left(\dfrac{\text{accuracy} - 0.05}{0.80 - 0.05},\, 0,\, 1\right)$, normalizing against the baseline (untrained) and target performance.

**Conditions that cause failure (true negatives):**
- Script crashes, times out, or produces no checkpoint.
- Checkpoint architecture does not match Qwen2-0.5B.
- Protected files were modified.
- Model fails to clear the accuracy threshold (bugs not fully fixed).

**Condition that causes success (true positives):**
- The LLM has identified and fixed the critical bugs, training converged within budget, and the model clears the accuracy threshold on held-out evaluation examples.

**Planted bugs (examples — all silent failures):**
1. **Loss-masking bug.** Labels are constructed by tokenizing the full prompt+completion sequence without setting prompt-token labels to `-100`. The model trains to predict the prompt as well as the completion, diluting the learning signal.
2. **EOS bug.** The EOS token is never appended to training completions. The model never learns to stop and generates garbage past the closing brace, breaking exact-match at eval time.
3. **Padding-label bug.** Pad-token positions are not set to `-100` in the label tensor, so the model trains to predict pad tokens as valid outputs.
4. **Gradient-accumulation bug.** `optimizer.zero_grad()` is called *inside* the accumulation loop instead of after the optimizer step, so each microbatch overwrites the previous gradient — effective batch size collapses to one microbatch.

Each bug alone is sufficient to prevent convergence. The LLM has to find and fix all of them (or at least the first three; the gradient-accumulation bug is the most subtle and most diagnostic of genuine understanding).

---

## Reward Hacking and Reward Denial Analysis

**Reward hacking — exploits considered and mitigated:**

| Potential exploit | Mitigation |
|---|---|
| LLM downloads a pretrained JSON-extraction model | No internet access; only `Qwen2-0.5B-Instruct` weights are cached locally. |
| LLM modifies `eval.py` or `judge.py` to inflate the score | Judge SHA-256 hashes both files before scoring. |
| LLM modifies the evaluation data | Hash check on `eval.jsonl`; judge also evaluates on its own private 200-example set the LLM never sees. |
| LLM trains on the judge's held-out eval set | Held-out set is never written to `/workspace/`. The LLM cannot see it. |
| LLM swaps in a different (stronger) base model | Judge inspects `config.json` and verifies the architecture matches Qwen2-0.5B. |
| LLM hardcodes the JSON output in `eval.py` via a wrapper | `eval.py` is protected and the judge constructs its own inference pipeline from it. |
| LLM uses few-shot prompting at inference instead of fine-tuning | The judge's inference path uses a fixed chat template with no few-shot examples; any few-shot logic the LLM adds to `train.py` does not affect the judge's eval prompts. |
| LLM cheats via prompt injection in the training data | Training data is hash-locked. Even if the LLM could inject, the judge's eval prompts are constructed from its own held-out set, not from anything in `/workspace/`. |

**Reward denial — failure modes for genuinely correct solutions, considered and mitigated:**

- *Threshold unachievable.* A correctly-implemented Qwen2-0.5B fine-tune on this task reaches >85% exact-match in <15 minutes on an A10. The 80% threshold leaves comfortable margin.
- *Bugs outside the LLM's modifiable scope.* All planted bugs live in files the LLM can edit. The model architecture and the dataset itself are untouched and correct.
- *Stochastic variance.* The judge's eval set is 200 examples and greedy decoding is deterministic; variance is negligible relative to the 80% threshold margin.
- *Tokenizer edge cases.* The task is constrained to a fixed schema with simple ASCII strings, eliminating tokenization edge cases that might cause exact-match to spuriously fail.

**Residual risk.** The continuous variant has mild drift — partial fixes can produce non-zero scores without solving the task. The binary variant eliminates this and is the form I would use for actual RL training signal; the continuous variant is useful only for diagnostic dashboards.

---

## Why I Chose This Environment

My background is in RL for robotics — I spent the last several months implementing PPO from scratch and training it on MuJoCo locomotion tasks, including silent debugging of failures where the loss curve looks reasonable but the policy never learns. The skill that mattered there was not anything robotics-specific: it was the ability to read a training loop while holding the algorithm in your head and notice where the implementation silently diverges from the math.

That is exactly the skill this environment puts in the foreground, just on LLM fine-tuning instead of policy optimization. The most common silent failures in supervised fine-tuning — wrong loss masking, missing EOS, broken gradient accumulation — are the supervised-learning analog of the silent PPO failures I have debugged firsthand (forgotten advantage normalization, terminal-state bootstrapping, action-distribution misconfiguration). Different mathematical objects, same diagnostic loop.

Three concrete design choices follow from that experience:

1. **The bugs are silent, not syntactic.** Syntactic bugs are trivially caught by a single training run. The interesting LLM-training debugging skill is mathematical, not procedural.
2. **The judge measures the actual fine-tuned model on a held-out distribution**, not a proxy like training loss. Anything proxy-based is gameable; "did the model actually learn the task" is not.
3. **The threshold is calibrated against a known-good reference run.** Knowing what a working Qwen2-0.5B fine-tune is supposed to look like at convergence makes it possible to set a threshold tight enough to require genuine fixes but loose enough to avoid penalizing minor implementation variance.

**Other environments I considered:**

- *Train a preference model from a pairwise-comparison dataset* (judged on held-out preference accuracy). Most directly aligned with the "Preference Model" role, but harder to make tamper-proof — preference accuracy can be inflated by training on the eval distribution.
- *Reproduce a paper's headline number* (e.g., LoRA fine-tuning on a small benchmark within X% of reported accuracy). Stronger as a "researcher" task, but introduces ambiguity around what counts as a faithful reproduction.
- *Inference-latency optimization*: cut a baseline `generate()` loop's latency 2× while holding output quality on a held-out prompt set. Concrete and gameable-resistant, but skews toward systems engineering more than ML research.

I picked the fine-tuning debug because (a) it is unambiguously an AI/ML engineering task, (b) the failure modes are well-studied and concrete enough to design around, and (c) it lets the design quality of the judge — the hardest part of this assignment — be the main thing being assessed.

---

## GitHub Profile Link

*(add your GitHub link here)*

Relevant repositories you may want to highlight:
- Your RL-from-scratch and PyTorch fundamentals work
- Any LLM fine-tuning or training-loop projects, if available

---

## Anything Else You Would Like Us to Know?

*(optional — leave blank, or note availability / start date / specific interest in the team's work)*
