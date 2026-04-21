# Lesson 6: Errors, Testing, And Debugging

## Goal

Handle failure intentionally and test behavior.

## Exceptions

```python
def parse_position(text: str) -> float:
    try:
        return float(text)
    except ValueError as exc:
        raise ValueError(f"invalid position: {text}") from exc
```

Add context when re-raising errors.

## Custom Exceptions

```python
class UnsafeCommandError(Exception):
    pass
```

Use custom exceptions when callers need to distinguish failure categories.

## Assertions

Assertions are useful for tests and internal sanity checks, but not user input validation.

```python
assert is_within_limits(5.0, 0.0, 10.0)
```

Python can disable assertions with optimization flags, so do not rely on them for safety checks.

## Testing With `pytest`

Example:

```python
def test_limit_accepts_boundary():
    limits = AxisLimits(0.0, 10.0)
    assert limits.contains(10.0)
```

Test unsafe behavior, not only happy paths.

## Exercises

1. Write tests for `AxisLimits.contains`.
2. Test exactly lower and upper boundaries.
3. Test command rejection outside limits.
4. Raise a custom `UnsafeCommandError`.
5. Read a traceback and identify the failing line.

## Pass Criteria

You pass when you can:

- use exceptions with useful messages;
- write focused tests;
- test boundary conditions;
- explain why assertions are not input validation;
- debug from traceback to root cause.

