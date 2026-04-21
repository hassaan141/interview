# Lesson 8: Iterators, Generators, And Context Managers

## Goal

Write memory-efficient data processing and reliable setup/cleanup code.

## Iterators

An iterator produces values one at a time.

```python
for line in file:
    process(line)
```

This avoids loading the entire file into memory.

## Generators

```python
def positions(samples: list[dict[str, str]]):
    for sample in samples:
        yield float(sample["position_mm"])
```

Generators are useful for streaming logs and sensor data.

## Generator Expressions

```python
total = sum(x for x in samples if x >= 0.0)
```

This can avoid creating intermediate lists.

## Custom Context Managers

```python
from contextlib import contextmanager

@contextmanager
def operation(name: str):
    print(f"begin {name}")
    try:
        yield
    finally:
        print(f"end {name}")
```

Context managers are Python's common setup/cleanup pattern.

## Exercises

1. Write a generator that streams positions from a CSV file.
2. Compute average position without storing all positions.
3. Write a context manager that logs start and end of an operation.
4. Explain why `with open(...)` is reliable.
5. Process a large fake log line by line.

## Pass Criteria

You pass when you can:

- explain iterator vs iterable;
- write a generator;
- use generator expressions;
- use context managers for cleanup;
- process data without unnecessary memory use.

