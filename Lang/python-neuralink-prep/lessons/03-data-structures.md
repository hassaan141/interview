# Lesson 3: Data Structures

## Goal

Use Python's core containers effectively and choose the right one.

## Lists

Lists are ordered, mutable sequences.

```python
samples = [1.0, 2.0, 3.0]
samples.append(4.0)
```

Use lists for ordered collections that may change.

## Tuples

Tuples are ordered and immutable.

```python
point = (12.0, 4.5)
```

Use tuples for fixed small groupings, especially when immutability is useful.

## Dictionaries

Dictionaries map keys to values.

```python
state_counts = {"Idle": 10, "Moving": 4}
state_counts["Faulted"] = 1
```

Use dictionaries for lookup by key.

## Sets

Sets store unique values.

```python
states_seen = {"Idle", "Moving"}
states_seen.add("Faulted")
```

Use sets for membership tests and uniqueness.

## Comprehensions

```python
unsafe = [x for x in samples if x > upper_limit]
```

Comprehensions are concise, but do not make them so dense that debugging becomes hard.

## Exercises

1. Store sensor samples in a list and compute min, max, and average.
2. Count robot states with a dictionary.
3. Build a set of unique error codes.
4. Use a list comprehension to find out-of-range samples.
5. Explain list vs tuple vs dict vs set.

## Pass Criteria

You pass when you can:

- choose core containers intentionally;
- use list operations;
- use dictionary lookup safely;
- explain mutability;
- avoid overly clever comprehensions.

