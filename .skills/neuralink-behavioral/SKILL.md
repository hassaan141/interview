---
name: neuralink-behavioral
description: Craft concise, high-signal behavioral interview answers for Neuralink robotics software roles. Use when Codex needs to answer, refine, rehearse, or critique recruiter screens, behavioral questions, "tell me about yourself," "why Neuralink," ownership/safety/reliability questions, or any response that should emphasize first-principles thinking, clear personal ownership, measurable impact, and low-noise communication for a busy interviewer.
---

# Neuralink Behavioral

Use this skill to generate answers that sound direct, thoughtful, and metric-backed rather than generic.

## Core Standard

Optimize every answer for these constraints:

- Lead with signal, not setup.
- Show first-principles thinking: state the real problem, constraint, or failure mode before the solution.
- Make personal ownership explicit: say what **I** did, not what the team generally did.
- Quantify impact whenever the repo supports it.
- Prefer the simplest reliable explanation over jargon.
- Keep recruiter-facing answers tight unless the user asks for a deeper version.
- Never invent metrics, technical scope, or medical-domain claims not supported by the source material.

## Source Of Truth

Before answering, ground the response in the repo notes. Start with:

- `Behavioural/resume_behavoiral.md`
- `Behavioural/general_behavioural.md`
- `Behavioural/Neuralink/neuralink_company.md`

Then pull supporting detail from the most relevant prep files listed in [references/interview-source-map.md](references/interview-source-map.md).

If different files disagree on a metric or count, prefer the most resume-like source or ask the user to choose before locking in a polished final version.

## Default Workflow

Follow this sequence unless the user asks for something narrower.

### 1. Pick the strongest story

Match the question to the best evidence.

- Safety, correctness, high stakes: `Rivian`, `Lincoln Electric`
- Real robot behavior, validation, sim-before-hardware: `Lincoln Electric`
- Robotics debugging under pressure, ambiguity, control problems: `WATOnomous`
- Reliability, performance, reproducibility: `WATOnomous`, `ISARA`, `Rivian`
- Data quality, practical ML limits: `AirMatrix`
- End-to-end ownership, integration, persistence: personal `SLAM ROS2 Robot`

Prefer stories with hard numbers and obvious user or system impact.

### 2. Reduce the story to first principles

State the answer in terms of:

1. What mattered.
2. What was blocking correctness, safety, speed, or reliability.
3. What decision you made and why that was the right tradeoff.
4. What measurable result changed.

Avoid starting with chronology if the underlying engineering principle is stronger.

### 3. Build the answer in this order

Use this structure for most behavioral responses:

1. `Situation`: one or two lines with only the context needed.
2. `Task`: the real responsibility or risk.
3. `Action`: what you personally did, how you did it, and how you reasoned from fundamentals.
4. `Impact`: the metric, blocker prevented, speedup, regression caught, or operational improvement.
5. `Reflection`: one line on what the experience taught you, only if it adds signal.

For recruiter screens, compress this into a short paragraph. For mock interviews, provide both a `60-second` and `2-minute` version when useful.

## Answer Templates

### Recruiter-tight template

Use this when the interviewer is busy and wants high signal fast:

`The core issue was <real constraint>. I owned <specific scope>. I approached it by <first-principles reasoning + concrete actions>. That led to <metric/result>.`

### Full behavioral template

Use this when the user wants a polished interview answer:

`In <context>, the key problem was <constraint/risk>. My responsibility was <ownership>. I decided to <decision> because <first-principles reason>. To do that, I <2-4 concrete actions>. The result was <metrics/impact>. What I learned was <short insight>.`

### Why Neuralink template

Anchor on:

- real-world impact on autonomy and human capability
- hard engineering with physical consequences
- high ownership in cross-functional robotics work
- preference for reliable systems over impressive demos

Do not drift into sci-fi language.

## Neuralink-Specific Framing

When relevant, emphasize these traits because they fit the role:

- calm debugging on systems tied to the physical world
- precision and repeatability over flashiness
- safety-aware engineering without sounding bureaucratic
- comfort working across software, robotics, hardware, and operators
- willingness to take ownership in ambiguous, high-pressure situations
- bias toward simple, reliable solutions

Translate prior work into Neuralink-relevant language carefully:

- `Rivian` -> safety-critical UI/software behavior, regression prevention, validation discipline
- `WATOnomous` -> robotics software, controls/perception debugging, sim infrastructure
- `Lincoln Electric` -> real robot motion, deployment validation, hardware consequences
- `ISARA` -> systematic debugging and reliability restoration

## Metrics Rule

Use numbers aggressively, but only if they are already supported in the repo material.

Good examples:

- `40` test cases
- `5` critical blockers caught
- `20%` of daily MRs protected from regressions
- `29%` latency reduction
- `85 to 120 FPS`
- `0.1 mm` IK convergence tolerance
- `100+` virtual weld validations
- `5-7 seconds per tack`
- `2-3 minutes per part`
- `355+` failures resolved
- `88.75%` product health improvement

If a number appears in one file but not another, keep the answer internally consistent inside the same conversation.

## Style Guardrails

Do:

- sound like an engineer who understands tradeoffs
- use clean, short sentences
- make ownership explicit
- explain technical work in plain English first
- tie impact to reliability, speed, safety, or user outcome

Do not:

- bury the answer under too much backstory
- overuse buzzwords like `passion`, `cutting-edge`, or `revolutionary`
- imply clinical or medical expertise you do not have
- exaggerate team leadership beyond the evidence
- answer with vague STAR filler that lacks technical substance

## Critique Mode

When the user gives a draft answer, review it against this checklist:

- Is the opening high-signal?
- Is the core problem stated clearly?
- Does it show first-principles reasoning?
- Is personal ownership obvious?
- Are the actions specific?
- Is the impact measurable?
- Is there any fluff to cut?

Prefer rewriting the answer, not just commenting on it.

## Output Modes

Offer the format that best fits the request:

- `Recruiter version`: 3-6 sentences
- `Interview version`: 45-90 seconds spoken
- `Deep version`: 1-2 minutes with more technical detail
- `Bullet cheat sheet`: situation, action, metrics, lesson

If the user does not specify, default to `Recruiter version` for initial passes.
