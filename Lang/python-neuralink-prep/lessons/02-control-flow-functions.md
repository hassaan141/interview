# Lesson 2: Control Flow And Functions

## Goal

Write decisions, loops, and reusable functions clearly.

## Conditionals

```python
if position_mm < lower_limit_mm:
    print("below lower limit")
elif position_mm > upper_limit_mm:
    print("above upper limit")
else:
    print("inside range")
```

Python uses indentation as syntax. Keep blocks small and readable.

## Loops

```python
for sample in samples:
    print(sample)
```

Use `range` when you need indices:

```python
for i in range(10):
    print(i)
```

Use `while` carefully:

```python
attempts = 0
while not homed and attempts < 100:
    attempts += 1
```

Never write a robot polling loop without a timeout or cancellation path.

## Functions

```python
def is_within_limits(position: float, lower: float, upper: float) -> bool:
    return lower <= position <= upper
```

Type hints are not required by the runtime, but they make design clearer and help tools catch mistakes.

## Exercises

1. Write `is_within_limits`.
2. Write `clamp_to_limits`.
3. Write a function that classifies a robot state string.
4. Process a list of positions and print unsafe values.
5. Write a polling loop with a timeout counter.

## Pass Criteria

You pass when you can:

- write `if`/`elif`/`else`;
- use `for` and `while` appropriately;
- define functions with clear names;
- add useful type hints;
- avoid unbounded loops.

