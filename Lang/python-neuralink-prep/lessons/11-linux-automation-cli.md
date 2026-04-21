# Lesson 11: Linux Automation And CLI Tools

## Goal

Build useful command-line tools for robotics workflows.

## `argparse`

```python
import argparse

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_path")
    parser.add_argument("--threshold-mm", type=float, default=0.5)
    return parser.parse_args()
```

Good CLIs validate input and fail clearly.

## Paths

Use `pathlib`:

```python
from pathlib import Path

path = Path("robot.log")
if not path.exists():
    raise FileNotFoundError(path)
```

## Subprocesses

```python
import subprocess

result = subprocess.run(["ls"], check=True, capture_output=True, text=True)
print(result.stdout)
```

Avoid `shell=True` unless there is a specific reason and you understand the risk.

## Logging

```python
import logging

logging.basicConfig(level=logging.INFO)
logging.info("started")
```

Use logging instead of scattered `print` calls for real tools.

## Exercises

1. Build a CLI that accepts an input file path.
2. Validate that the path exists.
3. Add a threshold argument.
4. Write output to a report file.
5. Run a subprocess safely without `shell=True`.

## Pass Criteria

You pass when you can:

- build a CLI with `argparse`;
- use `pathlib`;
- validate inputs;
- use `logging`;
- safely call subprocesses.

