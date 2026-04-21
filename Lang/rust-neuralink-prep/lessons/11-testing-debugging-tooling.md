# Lesson 11: Testing, Debugging, And Tooling

## Goal

Prove behavior with tests and use Rust tooling well.

## Unit Tests

```rust
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn accepts_boundary() {
        let limits = AxisLimits { lower_mm: 0.0, upper_mm: 10.0 };
        assert!(limits.contains(10.0));
    }
}
```

Run:

```bash
cargo test
```

## Tooling

Use:

```bash
cargo fmt
cargo clippy
cargo test
```

For debugging, build with debug symbols by default in dev profile:

```bash
cargo build
```

## Panics

Use `panic!` for unrecoverable programmer errors, not expected robot command rejection.

Expected failures should usually be `Result`.

## Exercises

1. Write unit tests for `AxisLimits`.
2. Test command rejection outside limits.
3. Test state transitions.
4. Run `cargo fmt`.
5. Run `cargo clippy` and address warnings.

## Pass Criteria

You pass when you can:

- write unit tests;
- test safety paths;
- use `cargo test`;
- distinguish panic from recoverable error;
- use formatters and linters.

