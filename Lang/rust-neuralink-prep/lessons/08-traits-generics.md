# Lesson 8: Traits And Generics

## Goal

Write reusable code with compile-time abstraction.

## Traits

Traits define shared behavior:

```rust
trait Sensor {
    fn read_position_mm(&self) -> f64;
}
```

Implement a trait:

```rust
struct FakeSensor {
    position_mm: f64,
}

impl Sensor for FakeSensor {
    fn read_position_mm(&self) -> f64 {
        self.position_mm
    }
}
```

## Generics

```rust
fn read_sensor<S: Sensor>(sensor: &S) -> f64 {
    sensor.read_position_mm()
}
```

This uses static dispatch by default.

## Trait Objects

```rust
fn read_dyn(sensor: &dyn Sensor) -> f64 {
    sensor.read_position_mm()
}
```

Trait objects use dynamic dispatch.

## Exercises

1. Define a `Sensor` trait.
2. Implement it for `FakeSensor`.
3. Write a generic function accepting `S: Sensor`.
4. Write a function accepting `&dyn Sensor`.
5. Explain static vs dynamic dispatch.

## Pass Criteria

You pass when you can:

- define and implement traits;
- write generic functions;
- use trait bounds;
- explain trait objects;
- choose static or dynamic dispatch intentionally.

