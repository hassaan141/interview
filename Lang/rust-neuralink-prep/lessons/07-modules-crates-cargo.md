# Lesson 7: Modules, Crates, And Cargo

## Goal

Organize Rust projects cleanly.

## Cargo Commands

```bash
cargo build
cargo run
cargo test
cargo fmt
cargo clippy
```

Use `cargo fmt` for formatting and `cargo clippy` for lints.

## Modules

```rust
mod limits;
```

This tells Rust to load `limits.rs` or `limits/mod.rs`.

## Visibility

Items are private by default. Use `pub` when exposing API:

```rust
pub struct AxisLimits {
    pub lower_mm: f64,
    pub upper_mm: f64,
}
```

Prefer exposing behavior over raw fields when invariants matter.

## Library And Binary

Common layout:

```text
src/lib.rs
src/main.rs
```

Put reusable logic in `lib.rs`; keep `main.rs` small.

## Exercises

1. Create a Cargo project.
2. Move `AxisLimits` into a module.
3. Expose only needed items with `pub`.
4. Add a `lib.rs` and keep `main.rs` small.
5. Run `cargo fmt` and `cargo clippy`.

## Pass Criteria

You pass when you can:

- use common Cargo commands;
- split code into modules;
- explain crate vs module;
- control visibility;
- separate library logic from executable entry point.

