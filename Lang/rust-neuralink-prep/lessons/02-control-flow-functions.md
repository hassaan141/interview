# Lesson 2: Control Flow And Functions

## Goal

Write decisions, loops, and functions in Rust.

## Conditionals

```rust
if position_mm < lower_limit_mm {
    println!("below lower limit");
} else if position_mm > upper_limit_mm {
    println!("above upper limit");
} else {
    println!("inside range");
}
```

`if` is an expression:

```rust
let status = if enabled { "enabled" } else { "disabled" };
```

## Loops

```rust
for sample in samples {
    println!("{sample}");
}
```

```rust
while attempts < 100 {
    attempts += 1;
}
```

Rust also has `loop`, which runs until `break`.

## Functions

```rust
fn is_within_limits(position: f64, lower: f64, upper: f64) -> bool {
    position >= lower && position <= upper
}
```

The final expression can be returned without `return` and without a semicolon.

## Exercises

1. Write `is_within_limits`.
2. Write `clamp_to_limits`.
3. Use `if` as an expression.
4. Write a loop with a timeout counter.
5. Explain the difference between expression and statement in Rust.

## Pass Criteria

You pass when you can:

- write `if`, `for`, `while`, and `loop`;
- write functions with return types;
- use expression returns;
- avoid unbounded loops;
- read compiler errors around semicolons and return types.

