# Lesson 1: Rust Foundations

## Goal

Understand Rust execution, Cargo, variables, mutability, and basic types.

## Cargo

Create a project:

```bash
cargo new robot_scratch
cd robot_scratch
cargo run
```

Cargo handles building, running, testing, dependencies, and project layout.

## Minimal Program

```rust
fn main() {
    println!("robot online");
}
```

`main` is the entry point.

## Variables

Variables are immutable by default:

```rust
let position_mm = 12.5;
```

Use `mut` only when mutation is needed:

```rust
let mut retry_count = 0;
retry_count += 1;
```

Immutability by default makes code easier to reason about.

## Basic Types

Common scalar types:

- `bool`
- `i32`, `i64`
- `u32`, `u64`
- `usize`
- `f32`, `f64`
- `char`

Use explicit types when units, layout, or API contracts matter:

```rust
let encoder_count: u32 = 1024;
let position_mm: f64 = 12.5;
```

## Exercises

1. Create a Cargo project.
2. Print your name, target role, and three Rust topics to improve.
3. Create immutable and mutable variables.
4. Try mutating an immutable variable and read the compiler error.
5. Store position, velocity, and enabled state with explicit types.

## Pass Criteria

You pass when you can:

- create and run a Cargo project;
- explain immutable by default;
- use `mut` intentionally;
- choose common numeric types;
- read a basic Rust compiler error.

