# Lesson 1: Python Foundations

## Goal

Understand Python execution, variables, basic types, and how to run scripts cleanly.

## Running Python

Run a file:

```bash
python3 main.py
```

Check version:

```bash
python3 --version
```

Python is interpreted through a runtime. Source code is not linked into a native executable the same way C++ is. That makes iteration fast, but runtime errors can appear only when a path is executed.

## Minimal Program

```python
print("robot tooling online")
```

## Variables

Python variables are names bound to objects.

```python
position_mm = 12.5
enabled = True
axis_name = "needle_axis"
```

Unlike C++, the name does not have a fixed declared type. The object has a type.

```python
print(type(position_mm))
```

## Basic Types

Common types:

- `bool`
- `int`
- `float`
- `str`
- `None`

Python integers can grow arbitrarily large. Floats are usually double-precision binary floating-point values.

## Style

Use clear names:

```python
velocity_mm_per_s = 4.2
```

For robotics code, include units in names when not using a stronger units system.

## Exercises

1. Write a script that prints your name, target role, and three Python topics to improve.
2. Store position, velocity, enabled state, and axis name in variables, then print them.
3. Print the type of each variable.
4. Intentionally create a `NameError`, read the traceback, then fix it.
5. Write a short note comparing Python runtime errors with C++ compile-time errors.

## Pass Criteria

You pass when you can:

- run a Python file from the terminal;
- explain names vs objects;
- use basic types;
- read a simple traceback;
- name variables with clear units.

