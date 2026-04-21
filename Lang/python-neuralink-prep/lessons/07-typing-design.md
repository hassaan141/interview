# Lesson 7: Type Hints And Design

## Goal

Use Python type hints to make code easier to reason about.

## Function Hints

```python
def tracking_error(commanded_mm: float, measured_mm: float) -> float:
    return abs(commanded_mm - measured_mm)
```

Type hints help readers and tools, but Python still runs dynamically.

## Optional Values

```python
def find_fault(samples: list[str]) -> str | None:
    for state in samples:
        if state == "Faulted":
            return state
    return None
```

Use `None` explicitly for absence.

## Literals And Enums

Prefer enums over arbitrary strings for known states:

```python
from enum import Enum

class AxisState(Enum):
    DISABLED = "disabled"
    IDLE = "idle"
    MOVING = "moving"
    FAULTED = "faulted"
```

## Protocols

Protocols define structural interfaces.

```python
from typing import Protocol

class Logger(Protocol):
    def info(self, message: str) -> None:
        ...
```

This is useful for testing because fake loggers can satisfy the same interface.

## Exercises

1. Add type hints to previous lesson functions.
2. Replace robot state strings with an `Enum`.
3. Write a function returning `float | None`.
4. Define a `Protocol` for a sensor reader.
5. Run a type checker if available, such as `mypy` or `pyright`.

## Pass Criteria

You pass when you can:

- write useful type hints;
- explain runtime vs static checking;
- use `None` intentionally;
- use `Enum` for known states;
- design small interfaces with `Protocol`.

