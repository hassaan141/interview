# Lesson 13: Concurrency

## Goal

Use Rust's concurrency tools safely.

## Threads

```rust
let handle = std::thread::spawn(|| {
    println!("worker");
});

handle.join().unwrap();
```

## Channels

Channels pass messages between threads.

```rust
use std::sync::mpsc;

let (tx, rx) = mpsc::channel();
tx.send("stop").unwrap();
let command = rx.recv().unwrap();
```

Message passing often produces simpler designs than shared mutable state.

## Shared State

```rust
use std::sync::{Arc, Mutex};

let value = Arc::new(Mutex::new(0));
```

`Arc` shares ownership across threads. `Mutex` controls mutation.

## Send And Sync

Rust uses marker traits:

- `Send`: a type can be moved to another thread.
- `Sync`: references to a type can be shared between threads.

These rules help prevent data races at compile time.

## Exercises

1. Spawn and join a thread.
2. Send commands over a channel.
3. Build a worker that receives `Command` enum values.
4. Use `Arc<Mutex<T>>` for shared counters.
5. Explain why Rust prevents many data races.

## Pass Criteria

You pass when you can:

- spawn threads;
- use channels;
- protect shared state;
- explain `Send` and `Sync`;
- design clean thread shutdown.

