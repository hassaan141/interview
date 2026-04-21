# Roadmap

## Interview Target

The Neuralink Robot Software role lists systems languages such as C, C++, and Rust. Rust is especially relevant for:

- memory safety without a garbage collector;
- explicit ownership;
- reliable error handling;
- safe concurrency;
- strong type modeling;
- systems code that must avoid undefined behavior.

This roadmap teaches Rust from fundamentals through robotics-style reliability.

## Suggested Pace

If you have 2 weeks:

- Days 1-3: lessons 1-4
- Days 4-6: lessons 5-8
- Days 7-9: lessons 9-11
- Days 10-12: lessons 12-14
- Days 13-14: final project and mock interview drills

If you have 4 weeks:

- Week 1: lessons 1-4
- Week 2: lessons 5-8
- Week 3: lessons 9-11
- Week 4: lessons 12-14 plus final project

## Daily Study Loop

Each day:

1. Read one lesson.
2. Create a small Cargo project for exercises.
3. Compile after every small change.
4. Read compiler diagnostics carefully.
5. Explain the ownership model of your code out loud.

## Final Project

Build:

`axis_controller_rs`

Requirements:

- Model a single robot axis with position, velocity, soft limits, state, and commands.
- Use enums for commands, states, and errors.
- Reject unsafe commands.
- Include homing, stopping, movement, and fault behavior.
- Use `Result` for command handling.
- Use tests for boundary and safety cases.
- Use a worker thread and channel for command processing.
- Avoid shared mutable state unless protected by synchronization.
- Keep allocation out of simulated control-loop hot paths where reasonable.

Pass standard:

- You can explain ownership of every major object.
- You can explain why invalid states are hard to represent.
- You can explain thread communication.
- You can explain where `unsafe` is not needed.
- Your tests cover normal behavior and unsafe rejection paths.

