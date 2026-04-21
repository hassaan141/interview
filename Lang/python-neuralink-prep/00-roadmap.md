# Roadmap

## Interview Target

For the Neuralink Robot Software internship, C++ or Rust is more likely to be central for production robot code. Python still matters because robotics teams commonly use it for:

- data analysis;
- test automation;
- robot operation tooling;
- simulation and prototyping;
- computer vision experiments;
- log processing;
- calibration scripts;
- CI and developer workflows.

This plan therefore teaches Python fundamentals first, then moves toward testing, typing, performance, automation, and robotics-style safety.

## Suggested Pace

If you have 2 weeks:

- Days 1-3: lessons 1-4
- Days 4-6: lessons 5-8
- Days 7-9: lessons 9-10
- Days 10-12: lessons 11-12
- Days 13-14: final project and mock interview drills

If you have 4 weeks:

- Week 1: lessons 1-4
- Week 2: lessons 5-8
- Week 3: lessons 9-10
- Week 4: lessons 11-12 plus final project

## Daily Study Loop

Each day:

1. Read one lesson.
2. Recreate examples from memory.
3. Solve exercises.
4. Run tests.
5. Explain the topic out loud.

## Final Project

Build:

`robot_log_analyzer`

Requirements:

- Read a CSV or JSONL robot log containing timestamps, commanded position, measured position, velocity, state, and error flags.
- Validate input schema.
- Detect limit violations, stale sensor samples, position tracking error, and unexpected state transitions.
- Produce a summary report.
- Include tests for valid logs and unsafe logs.
- Provide a CLI with arguments for input path, output path, and thresholds.
- Use type hints.
- Use dataclasses for structured records.
- Use context managers for file handling.

Pass standard:

- You can explain every data structure choice.
- You can explain error handling paths.
- You can test both normal and unsafe logs.
- You can describe where Python is appropriate and where C++/Rust would be better.
- Your CLI fails with useful messages on invalid inputs.

