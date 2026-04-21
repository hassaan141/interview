# Lesson 4: Structs, Enums, And Pattern Matching

## Goal

Model data and state explicitly.

## Structs

```rust
struct AxisLimits {
    lower_mm: f64,
    upper_mm: f64,
}

impl AxisLimits {
    fn contains(&self, position_mm: f64) -> bool {
        position_mm >= self.lower_mm && position_mm <= self.upper_mm
    }
}
```

`impl` blocks define methods.

## Enums

```rust
enum AxisState {
    Disabled,
    Idle,
    Moving,
    Homing,
    Faulted,
}
```

Enums are excellent for state machines.

## Enums With Data

```rust
enum Command {
    MoveAbsolute { target_mm: f64 },
    Stop,
    Home,
}
```

This makes command data explicit and type-checked.

## Pattern Matching

```rust
match command {
    Command::MoveAbsolute { target_mm } => println!("move to {target_mm}"),
    Command::Stop => println!("stop"),
    Command::Home => println!("home"),
}
```

`match` is exhaustive. The compiler checks that all variants are handled.

## Exercises

1. Implement `AxisLimits`.
2. Implement `AxisState`.
3. Implement `Command`.
4. Write a `handle_command` function using `match`.
5. Add a new command variant and observe compiler feedback.

## Pass Criteria

You pass when you can:

- define structs and methods;
- use enums for states and commands;
- pattern match command data;
- explain exhaustive matching;
- model invalid states out of existence.

