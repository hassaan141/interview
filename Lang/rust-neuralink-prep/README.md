# Rust Refresh For Robotics SWE Interviews

This curriculum is a beginner-to-advanced Rust refresh for robotics and high-reliability systems interviews. Rust is relevant to the target role because it is a systems language with strong compile-time guarantees around ownership, lifetime, memory safety, and concurrency.

The necessary number of files is 16:

- `README.md`: overview and how to use the curriculum.
- `00-roadmap.md`: schedule, expectations, and final project.
- `lessons/01-foundations.md`
- `lessons/02-control-flow-functions.md`
- `lessons/03-ownership-borrowing.md`
- `lessons/04-structs-enums-patterns.md`
- `lessons/05-error-handling.md`
- `lessons/06-collections-iterators.md`
- `lessons/07-modules-crates-cargo.md`
- `lessons/08-traits-generics.md`
- `lessons/09-lifetimes.md`
- `lessons/10-smart-pointers-resource-management.md`
- `lessons/11-testing-debugging-tooling.md`
- `lessons/12-performance-systems.md`
- `lessons/13-concurrency.md`
- `lessons/14-unsafe-ffi-embedded.md`

## How To Study

For each lesson:

1. Read the explanation.
2. Type the examples by hand.
3. Compile frequently.
4. Treat compiler errors as feedback, not failure.
5. Complete the exercises and pass criteria.

Rust requires patience early because the compiler forces you to be precise. That precision is exactly why Rust is useful for reliable systems.

## Expected Outcome

By the end, you should be able to:

- explain ownership, borrowing, lifetimes, and moves;
- write safe Rust with explicit error handling;
- use structs, enums, traits, generics, and iterators;
- organize code with Cargo modules and crates;
- write tests and use standard tooling;
- reason about performance and allocation;
- write safe concurrent code;
- understand where `unsafe` and FFI fit.

