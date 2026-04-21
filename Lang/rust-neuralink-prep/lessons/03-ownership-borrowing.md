# Lesson 3: Ownership And Borrowing

## Goal

Understand Rust's central safety model: ownership, moves, borrowing, and references.

## Ownership Rules

Rust ownership has three core rules:

1. Each value has one owner.
2. There can only be one owner at a time.
3. When the owner goes out of scope, the value is dropped.

## Move

```rust
let name = String::from("axis");
let other = name;
// name is no longer usable here
```

`String` owns heap memory, so assigning it moves ownership.

## Copy

Simple types like integers usually implement `Copy`:

```rust
let a = 5;
let b = a;
println!("{a} {b}");
```

Both remain usable because the value is copied.

## Borrowing

Borrow immutably:

```rust
fn print_name(name: &str) {
    println!("{name}");
}
```

Borrow mutably:

```rust
fn zero(value: &mut f64) {
    *value = 0.0;
}
```

Rule: many immutable references or one mutable reference, but not both at the same time.

## Robotics Relevance

Ownership forces you to answer:

- who owns this buffer?
- who may mutate this state?
- can another thread access this?
- when does cleanup happen?

Those questions matter for reliable robot software.

## Exercises

1. Move a `String` and observe the compiler error when using the old name.
2. Copy an `i32` and explain why both names work.
3. Write a function that borrows a string.
4. Write a function that mutably borrows a position and resets it.
5. Trigger and fix a mutable/immutable borrow conflict.

## Pass Criteria

You pass when you can:

- explain ownership rules;
- explain move vs copy;
- use immutable and mutable references;
- explain why Rust prevents dangling references;
- connect borrowing to safe shared state.

