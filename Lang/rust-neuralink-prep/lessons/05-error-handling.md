# Lesson 5: Error Handling

## Goal

Handle expected failures explicitly with `Option` and `Result`.

## Option

`Option<T>` represents value or no value:

```rust
fn find_fault(states: &[AxisState]) -> Option<AxisState> {
    // example only if AxisState is Copy or Clone
    None
}
```

Variants:

- `Some(value)`
- `None`

## Result

`Result<T, E>` represents success or error:

```rust
enum CommandError {
    Disabled,
    OutsideLimits,
}

fn validate_move(enabled: bool, target: f64, limits: &AxisLimits) -> Result<(), CommandError> {
    if !enabled {
        return Err(CommandError::Disabled);
    }
    if !limits.contains(target) {
        return Err(CommandError::OutsideLimits);
    }
    Ok(())
}
```

## `?` Operator

The `?` operator returns early on error:

```rust
fn process() -> Result<(), CommandError> {
    validate_move(true, 5.0, &AxisLimits { lower_mm: 0.0, upper_mm: 10.0 })?;
    Ok(())
}
```

## No Exceptions

Rust does not use exceptions for normal error handling. Expected failures are values.

## Exercises

1. Write `validate_move`.
2. Add error variants for `AlreadyMoving` and `NotHomed`.
3. Use `match` to print error messages.
4. Use `?` in a function returning `Result`.
5. Explain why explicit errors help safety-critical code.

## Pass Criteria

You pass when you can:

- use `Option`;
- use `Result`;
- define custom error enums;
- use `?`;
- avoid panics for expected failures.

