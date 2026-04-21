# Lesson 9: Performance, NumPy, And Data

## Goal

Understand Python performance limits and how to process numeric data effectively.

## Python Performance Reality

Pure Python loops are often slower than C++ or Rust loops because each operation has dynamic runtime overhead.

This does not make Python bad. It means you should use Python where it is strong:

- orchestration;
- analysis;
- test tooling;
- data pipelines;
- fast iteration.

For heavy numeric arrays, use libraries implemented in optimized native code.

## NumPy

```python
import numpy as np

positions = np.array([1.0, 2.0, 3.0])
mean_position = positions.mean()
```

Vectorized operations run in optimized lower-level code.

## Avoid Premature Optimization

First:

1. make it correct;
2. test it;
3. measure it;
4. optimize the measured bottleneck.

## Timing

```python
from time import perf_counter

start = perf_counter()
# work
elapsed = perf_counter() - start
```

Use realistic input sizes.

## Exercises

1. Time summing one million numbers with a Python loop.
2. Time summing the same data with NumPy.
3. Detect samples where absolute tracking error exceeds a threshold.
4. Compare storing samples as dictionaries vs dataclasses.
5. Explain when Python is the wrong language for a control loop.

## Pass Criteria

You pass when you can:

- explain why Python loops can be slow;
- use NumPy for numeric arrays;
- measure with `perf_counter`;
- avoid optimizing without data;
- identify workloads better suited to C++ or Rust.

