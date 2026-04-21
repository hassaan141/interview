# Lesson 12: Performance And Systems Thinking

## Goal

Reason about allocation, copying, layout, and measurement in Rust.

## Zero-Cost Abstractions

Rust often compiles high-level abstractions into efficient machine code. Iterators, generics, and pattern matching can be fast when used well.

Do not assume. Measure.

## Allocation

Heap allocation occurs with types such as:

- `String`
- `Vec<T>`
- `Box<T>`
- `Arc<T>`

For hot loops, preallocate:

```rust
let mut samples = Vec::with_capacity(1024);
```

## Borrow To Avoid Copies

```rust
fn average(samples: &[f64]) -> f64 {
    samples.iter().sum::<f64>() / samples.len() as f64
}
```

Slices allow borrowing contiguous data without taking ownership.

## Timing

```rust
let start = std::time::Instant::now();
// work
let elapsed = start.elapsed();
```

Use realistic workloads.

## Exercises

1. Compare pushing into `Vec::new()` vs `Vec::with_capacity`.
2. Write a function that accepts `&[f64]`.
3. Time a loop with `Instant`.
4. Avoid cloning a `String` unnecessarily.
5. Explain why deterministic allocation matters in control loops.

## Pass Criteria

You pass when you can:

- identify heap-allocating types;
- use slices;
- avoid unnecessary clones;
- preallocate when appropriate;
- measure performance claims.

