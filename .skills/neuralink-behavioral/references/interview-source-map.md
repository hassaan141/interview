# Interview Source Map

Use this file to quickly choose which repo notes to load before writing an answer.

## Primary files

- `Behavioural/resume_behavoiral.md`
- `Behavioural/general_behavioural.md`
- `Behavioural/Neuralink/neuralink_company.md`

These are the default grounding files for almost every answer.

## Story bank by employer or project

### Rivian

Use for:

- safety-critical behavior
- regression prevention
- validation discipline
- cross-functional collaboration with ADAS and infotainment
- moving quality checks earlier in the pipeline

Read:

- `Behavioural/resume_behavoiral.md`
- `Behavioural/general_behavioural.md`
- `Behavioural/Rivian/general_behavioural.md` if present

Key metrics commonly referenced:

- `40` test cases
- `5` blockers caught
- `40` automated navigation flows
- `30 min` APK pipeline replacing a `3-hour` image build
- regressions prevented in `20%` of daily MRs

### WATOnomous

Use for:

- robotics debugging
- ambiguity
- controls and IK
- perception performance
- sim-to-real lessons
- independent technical ownership

Read:

- `Behavioural/resume_behavoiral.md`
- `Behavioural/general_behavioural.md`
- `Behavioural/isaacsim_interview_questions.md`

Key metrics commonly referenced:

- `29%` latency reduction
- `85 to 120 FPS`
- `4x` mapping pipeline speedup
- `0.1 mm` IK convergence tolerance

### Lincoln Electric

Use for:

- software affecting real robot motion
- deployment validation
- precision and repeatability
- simulation before touching hardware
- operator workflow simplification

Read:

- `Behavioural/general_behavioural.md`
- `Behavioural/LincolnElectric_Interview_Prep.pdf` derived notes if already absorbed in conversation

Key metrics commonly referenced:

- `100+` virtual welds
- `5-7 seconds` saved per tack
- `2-3 minutes` saved per part
- `2` production cells

### ISARA

Use for:

- systematic debugging
- reliability improvement
- QA partnership
- backlog reduction
- automation and developer efficiency

Read:

- `Behavioural/general_behavioural.md`
- `Behavioural/ISARA_AirMatrix_Interview_Prep.pdf` derived notes if already absorbed in conversation

Key metrics commonly referenced:

- `355+` failures resolved
- `88.75%` product health improvement
- `10 minutes` saved per deployment

### AirMatrix

Use for:

- data quality
- practical ML tradeoffs
- honest model limitations
- annotation discipline

Read:

- `Behavioural/general_behavioural.md`
- `Behavioural/ISARA_AirMatrix_Interview_Prep.pdf` derived notes if already absorbed in conversation

Key metrics commonly referenced:

- `3,000+` images annotated
- `95%` accuracy at `0.5 IoU`

### Personal robotics

Use for:

- end-to-end ownership
- learning fast
- integration across hardware and software
- persistence on technically hard problems

Read:

- `Behavioural/general_behavioural.md`
- resume project bullets in `Behavioural` PDFs or markdown notes

## Answer selection heuristics

- Choose the story with the clearest consequence.
- If two stories fit, prefer the one with stronger metrics.
- For Neuralink recruiter screens, prefer `Rivian` or `Lincoln Electric` first because the stakes and impact are easiest to communicate quickly.
- For robotics software interviewers, prefer `WATOnomous` or `Lincoln Electric` when the question is about debugging, controls, or real robot behavior.
