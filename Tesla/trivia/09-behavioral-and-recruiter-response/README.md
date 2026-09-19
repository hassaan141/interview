# 09 — Behavioral Prep and the Recruiter's Email

Two different things live here:
1. **`recruiter-reply.md`** — a template for answering the recruiter's bullet points.
2. **This file** — behavioral prep: the stories to prepare, the Tesla-specific questions,
   and what to ask them.

Nothing here invents anything about you. Every story slot is a **prompt** pointing at work
you actually have in this repo; you fill in the specifics.

---

## Part 1 — The three stories you must have ready

Interviewers ask maybe six behavioral questions, but almost all of them are one of three
stories retold. Prepare these three properly and you can answer nearly anything.

Use **STAR**, and weight it correctly: about 20% Situation, 10% Task, **50% Action**, 20%
Result. Candidates spend too long on setup and never say what *they personally did*.

### Story A — "The hardest technical problem you solved"
This is the one that matters most for this role. Pick something with a **debugging or
performance** shape, because that is what the job is.

Fill in:
- **Situation** (2 sentences): what system, what was broken or too slow, why it mattered.
- **Task**: what specifically was *yours* to solve.
- **Action** (the bulk): what you *measured*, what hypotheses you formed and **rejected**,
  what you changed, and why. Name the tools. The interviewer is listening for a *method*,
  not a miracle.
- **Result**: a number if you have one (latency, throughput, crash rate, build time), and
  what you would do differently.

Candidate sources in this repo — pick the one with the most real detail:
- the WATonomous humanoid arm work (real hardware, real failure modes, real safety
  constraints — this is the strongest kind of story for Tesla)
- `ros-tuts/` (ROS2 workspaces, custom interfaces, build/integration issues)
- `RL/`, `Pytorch/` (training pipelines, reproducibility, performance)
- `RobotMath/` (numerical/geometry correctness)

**If the story is about the robot arm, lead with safety.** A candidate who says "we almost
damaged the hardware, so I added X and Y before anyone ran anything on the real arm" is
exactly the instinct an autonomy team wants.

### Story B — "A time you worked with others" (conflict, disagreement, or teaching)
Tesla-flavoured version: you disagreed with a technical decision, or you had to get someone
else to change their approach. What they listen for: did you argue from **evidence**, did
you actually change your mind when shown data, and did the relationship survive.

Fill in:
- What the disagreement was, in one sentence, **stated fairly from both sides**.
- What evidence you brought (a measurement, a failure case, a prototype).
- How it resolved — including the case where *you* were wrong. "I was wrong and here is what
  changed my mind" is a strong answer, not a weak one.

### Story C — "A time you failed, or shipped something that broke"
The trap is picking a non-failure ("I worked too hard"). Pick a real one, own it without
drama, and spend most of the time on **what you changed in how you work**.

Fill in:
- What broke, and what the actual consequence was.
- Your specific contribution to it (not the team's).
- The **systemic** fix, not the one-off: a test, a check, a review step, an assertion.

---

## Part 2 — Questions you will probably get

Behavioral:
1. Tell me about yourself. *(90 seconds. Not your life story — a path that ends at "which is
   why this role.")*
2. Why Tesla? Why this team specifically?
3. Tell me about the most technically difficult thing you have built.
4. Tell me about a bug that took you a long time to find.
5. A time you disagreed with a teammate or a decision.
6. A time you failed or broke something.
7. A time you had to learn something quickly.
8. How do you handle an ambiguous or under-specified task?
9. What do you do when you are blocked?
10. Tell me about a time you improved something nobody asked you to improve.

Role-specific "behavioral-technical" (these are the ones that actually differentiate):
11. How do you approach making unfamiliar code faster?
12. How do you decide something is fast enough?
13. You are asked to add a feature to a library 40 teams depend on. What is your process?
14. How do you review someone else's C++?
15. How would you introduce a practice (sanitizers in CI, say) to a team that is not using it?
16. What is your opinion on comments? On tests? On code review?
17. What is the last technical thing you changed your mind about?

### How to answer "Why Tesla / why this team"
Be specific to the posting, not to the brand. Something like: *"The posting is a foundations
role — transport, tooling, benchmarking, and test infrastructure that other teams build on.
That is the work I like: I would rather make fifty people's code faster and harder to get
wrong than write one feature. And the constraint is real here — a fixed compute budget and a
hard latency deadline on a chip you cannot just scale up — which makes the performance work
actually matter rather than being an optimization exercise."* Then connect it to one concrete
thing you have done.

Do **not** say "I love the mission" and stop there. Everyone says that.

### How to answer "what are you weak at"
Name a real gap **and the specific thing you are doing about it**. You have an obvious,
honest one available: *"My C++ depth was behind my systems intuition, so I worked through the
Modern C++ Programming course end to end, wrote runnable examples for every chapter, and
benchmarked the parts I only knew as folklore — I can tell you what a cache miss actually
costs on my machine because I measured it."* That answer is strong precisely because it is
verifiable and recent.

---

## Part 3 — Questions to ask them

Ask three or four. Good questions are specific and show you read the posting.

**About the work**
1. What does the foundations team own today, and what is the boundary with the application
   teams?
2. What is the current biggest source of latency in the stack — is it compute, data
   movement, or synchronization?
3. Is the evaluation pipeline deterministic today? What breaks replay most often?
4. What does the benchmarking tooling look like now, and what is missing from it?
5. How much of the stack runs on the FSD chip's accelerator vs. the CPU cores, and what
   decides that split?

**About the team**
6. What would I be expected to have shipped by the end of the internship?
7. How does code get from my branch to a vehicle? What is in between?
8. What is the review culture like — how do you catch a performance regression before it
   lands?
9. What is the hardest part of working on this team that is not obvious from outside?

**About the intern specifically**
10. Is there an intern project already scoped, or would I help scope it?
11. Who would I be pairing with, and how often?

**Do not ask**: anything answerable from the job posting, compensation in a technical round,
or "what does a typical day look like" (too generic to produce a useful answer).

---

## Part 4 — Logistics to have straight before the call

- Your **availability window** and whether you can do Spring (Jan 2027) or Summer (May 2027)
  or both.
- Your **graduation date** and degree.
- **Work authorization** status and whether you need sponsorship, stated plainly.
- **Location** willingness (Palo Alto / Austin / relocation).
- A two-sentence description of your most relevant project, ready to say without thinking.
- Your GitHub link, if you want them to see this repo.

See `recruiter-reply.md` for a template.

---

## Part 5 — The hour before the interview

- Re-read your own **three stories** — out loud, once each.
- Re-read `cpp-course/03` (memory), `cpp-course/09` (move semantics), and `cpp-course/14`
  (performance) traps checklists. Those three cover most of what gets asked.
- Have an editor open with a blank `.cpp` file and your compile command ready.
- Have `leetcode/17-design-and-systems-questions/README.md` open in a tab — if you get a
  design question, it is probably one of those seven.
- Write down the three numbers: **DRAM ≈ 100x L1**, **cache line = 64 bytes**, **mispredict
  ≈ 15-20 cycles**. If you can only remember three facts, remember those.

## Part 6 — During the technical round

1. **Restate the problem** in your own words, and ask about the constraints that actually
   change the answer: input size, whether you can mutate the input, memory budget,
   single-threaded or not, real-time or offline.
2. **Say the approach before you write it.** State the complexity you are aiming for.
3. **Think out loud.** Silence reads as being stuck; narration reads as competence, and it
   lets the interviewer redirect you before you waste ten minutes.
4. **Write compilable C++**, not pseudocode. Use `std::` properly. `const&` your parameters.
5. **Test it out loud** on the empty input, the single element, and the boundary — before
   they ask.
6. **State the complexity and the memory access pattern** when you finish. For this team,
   adding "this allocates once up front and never in the loop" is worth real points.
7. If you are stuck, say what you have ruled out and why. That is a data point in your
   favour, not against you.
8. If you realize you were wrong, say so immediately and correct it. Do not defend a broken
   approach.
