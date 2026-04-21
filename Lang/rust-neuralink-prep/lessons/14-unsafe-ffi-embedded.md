# Lesson 14: Unsafe, FFI, Embedded, And Robotics Safety

## Goal

Understand where Rust's safety boundary is and how it relates to hardware and existing C/C++ systems.

## Unsafe

`unsafe` allows operations the compiler cannot fully verify, such as:

- dereferencing raw pointers;
- calling unsafe functions;
- accessing mutable statics;
- implementing unsafe traits;
- interacting with foreign code.

`unsafe` does not turn off the borrow checker everywhere. It creates a small region where you must uphold extra rules manually.

## FFI

Rust often interfaces with C APIs:

```rust
unsafe extern "C" {
    fn some_c_function(value: i32) -> i32;
}
```

FFI requires careful handling of:

- pointer validity;
- ownership transfer;
- allocation and deallocation boundaries;
- thread safety;
- error conventions.

## Hardware And Embedded Thinking

Robotics systems may interact with:

- device files;
- serial ports;
- field buses;
- microcontrollers;
- sensors;
- actuators.

Rust can wrap unsafe hardware access in safe APIs. The public API should make invalid or unsafe use difficult.

## Safety-Critical Modeling

Use types to encode rules:

```rust
struct Millimeters(f64);
struct MillimetersPerSecond(f64);
```

Use enums for states and commands. Use `Result` for rejected operations. Use tests for every unsafe path.

## Final Exercises

1. Build the `axis_controller_rs` project from the roadmap.
2. Model units with newtypes.
3. Model commands and states with enums.
4. Reject invalid transitions.
5. Add tests for unsafe commands.
6. Write a design note explaining where `unsafe` would be isolated if hardware access were added.

## Mock Interview Questions

1. Explain ownership and borrowing.
2. Explain how Rust prevents data races.
3. When would you use `Arc<Mutex<T>>`?
4. Why is `Result` useful in robot command handling?
5. How would you model a robot state machine in Rust?
6. What does `unsafe` mean?
7. How would Rust call into a C motor driver?
8. How do you avoid allocation in a hot control loop?

## Pass Criteria

You pass when you can:

- explain Rust's safety boundary;
- describe FFI risks;
- wrap unsafe behavior behind safe APIs;
- model robot safety rules in types;
- connect ownership and concurrency to physical robot reliability.

