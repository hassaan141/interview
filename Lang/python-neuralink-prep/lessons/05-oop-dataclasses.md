# Lesson 5: OOP And Dataclasses

## Goal

Model structured data and behavior with classes and dataclasses.

## Dataclasses

Use dataclasses for simple structured records.

```python
from dataclasses import dataclass

@dataclass(frozen=True)
class AxisLimits:
    lower_mm: float
    upper_mm: float

    def contains(self, position_mm: float) -> bool:
        return self.lower_mm <= position_mm <= self.upper_mm
```

`frozen=True` makes instances immutable after construction.

## Classes With Invariants

```python
class AxisController:
    def __init__(self, limits: AxisLimits) -> None:
        if limits.lower_mm > limits.upper_mm:
            raise ValueError("lower limit exceeds upper limit")
        self._limits = limits
        self._enabled = False
```

Private-by-convention attributes start with `_`.

## Properties

Use properties for read-only access when needed:

```python
@property
def enabled(self) -> bool:
    return self._enabled
```

Do not add trivial getters and setters automatically. Expose behavior when possible.

## Exercises

1. Create a frozen `AxisLimits` dataclass.
2. Create a `RobotSample` dataclass with timestamp, commanded position, measured position, and state.
3. Implement an `AxisController` with `enable`, `disable`, and `move_absolute`.
4. Reject moves outside limits.
5. Explain when a dataclass is better than a dictionary.

## Pass Criteria

You pass when you can:

- define classes and dataclasses;
- protect simple invariants;
- use immutability where helpful;
- avoid exposing all internals directly;
- model robotics records clearly.

