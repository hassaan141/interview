# Lesson 4: Files, Modules, And Packages

## Goal

Organize Python code and read/write files safely.

## Context Managers

Always prefer context managers for file handling:

```python
with open("robot.log", "r", encoding="utf-8") as file:
    contents = file.read()
```

The file closes automatically, even if an exception occurs.

## CSV

```python
import csv

with open("samples.csv", newline="", encoding="utf-8") as file:
    reader = csv.DictReader(file)
    for row in reader:
        print(row["position_mm"])
```

## JSON

```python
import json

with open("config.json", "r", encoding="utf-8") as file:
    config = json.load(file)
```

## Modules

A module is a `.py` file that can be imported.

```python
from limits import is_within_limits
```

Keep reusable logic in modules and script entry points small.

## Entry Point

```python
def main() -> int:
    print("running")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
```

This makes code importable without running the script body.

## Exercises

1. Read a text file with `with open`.
2. Read a CSV containing timestamps and positions.
3. Write a JSON summary file.
4. Split a helper function into another module.
5. Use a `main()` function and `raise SystemExit(main())`.

## Pass Criteria

You pass when you can:

- use context managers for files;
- read CSV and JSON;
- organize code into modules;
- explain `if __name__ == "__main__"`;
- keep side effects out of imports.

