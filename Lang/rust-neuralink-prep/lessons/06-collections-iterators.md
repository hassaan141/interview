# Lesson 6: Collections And Iterators

## Goal

Use Rust collections and iterator pipelines effectively.

## Vec

```rust
let mut samples = Vec::new();
samples.push(1.0);
samples.push(2.0);
```

`Vec<T>` is a contiguous growable array.

## HashMap

```rust
use std::collections::HashMap;

let mut counts = HashMap::new();
counts.insert("Idle", 3);
```

## Iterators

```rust
let count = samples.iter().filter(|x| **x > 10.0).count();
```

`iter()` borrows elements. `into_iter()` consumes the collection. `iter_mut()` mutably borrows elements.

## Ownership In Iteration

```rust
for value in samples.iter() {
    println!("{value}");
}
```

This keeps `samples` usable afterward.

## Exercises

1. Store sensor samples in a `Vec<f64>`.
2. Compute min, max, and average.
3. Count out-of-range samples with iterators.
4. Count state strings with `HashMap`.
5. Explain `iter`, `iter_mut`, and `into_iter`.

## Pass Criteria

You pass when you can:

- use `Vec`;
- use `HashMap`;
- write iterator chains;
- explain borrowing during iteration;
- avoid unnecessary clones.

