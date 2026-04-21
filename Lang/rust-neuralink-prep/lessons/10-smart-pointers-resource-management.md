# Lesson 10: Smart Pointers And Resource Management

## Goal

Understand heap ownership, shared ownership, interior mutability, and deterministic cleanup.

## Box

`Box<T>` stores a value on the heap with one owner.

```rust
let value = Box::new(42);
```

Use `Box` for heap allocation, recursive types, or trait objects.

## Rc And Arc

`Rc<T>` is reference-counted shared ownership for single-threaded code.

`Arc<T>` is atomic reference-counted shared ownership for multi-threaded code.

Shared ownership should be justified because it makes lifetime less obvious.

## Mutex

```rust
use std::sync::Mutex;

let value = Mutex::new(0);
```

`Mutex<T>` protects shared mutable data.

## Drop

Rust automatically runs `Drop` when an owner goes out of scope. This is similar in spirit to RAII in C++.

Files, locks, sockets, and other resources should be tied to ownership so cleanup is deterministic.

## Exercises

1. Use `Box<i32>`.
2. Explain why `Box` is single-owner.
3. Use `Arc<Mutex<T>>` for shared state between threads.
4. Write a small type with a `Drop` implementation that logs cleanup.
5. Explain why shared ownership can complicate design.

## Pass Criteria

You pass when you can:

- use `Box`;
- explain `Rc` vs `Arc`;
- use `Mutex`;
- explain deterministic cleanup;
- avoid unnecessary shared ownership.

