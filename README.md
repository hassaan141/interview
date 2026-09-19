# Interview Prep

This repository is organized by study area. Start with the smallest relevant
folder instead of browsing the repository root.

## Study areas

- [`Tesla`](Tesla/README.md) — **current focus**: the Tesla C++ interview (Autonomy
  Systems Foundations internship). A 4-week plan, the *Modern C++ Programming* lecture
  series broken into 16 sections with runnable examples and quizzes, 105 LeetCode
  solutions in modern C++, and job-specific trivia.
- [`RobotMath/ModernRobotics`](RobotMath/ModernRobotics/README.md) — robotics math and
  the *Modern Robotics* course.
- `RobotMath/RobotControl` — robot-control coding questions and practice.
- `leetcode` — data structures and algorithms.
- `RL` — reinforcement learning, MuJoCo, locomotion, and RL for LLMs.
- `Pytorch` — PyTorch notes, exercises, and projects.
- `Lang` — language-specific interview preparation.
- `Behavioural` — company-specific and general behavioral preparation.
- `Trivia` — short software-engineering and robotics review notes.
- `ai_assisted` — AI-assisted take-home and system-design prompts.
- `ros-tuts` — ROS tutorials and workspaces.

## Repository conventions

- Put generated models, plots, logs, and checkpoints in an `artifacts/`,
  `checkpoints/`, or `runs/` folder beside the code that produced them.
- Keep learning notes inside the subject folder, not at the repository root.
- Keep the existing `venv` folder local; it is ignored by Git.
- Do not commit generated outputs or operating-system metadata.

## Current next step

Open the [Tesla interview plan](Tesla/README.md) and start with
`Tesla/cpp-course/03-memory-pointers-references-and-const/` — it is the highest-yield
folder for the technical round. Everything under `Tesla/` compiles and runs:

```bash
bash Tesla/build_all.sh      # builds and runs every example and solution
```

After the interview, the previous focus was the
[Modern Robotics study guide](RobotMath/ModernRobotics/README.md), Chapter 2 onward.
