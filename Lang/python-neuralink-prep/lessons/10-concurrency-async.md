# Lesson 10: Concurrency And Async

## Goal

Understand Python threading, multiprocessing, async IO, and the GIL at a practical level.

## GIL

CPython has a Global Interpreter Lock that limits execution of Python bytecode by multiple threads at once. This affects CPU-bound Python code.

Threads can still be useful for IO-bound work:

- reading files;
- network requests;
- waiting for devices;
- running subprocesses.

## Threading

```python
from threading import Thread

def worker() -> None:
    print("work")

thread = Thread(target=worker)
thread.start()
thread.join()
```

Use locks when sharing mutable data.

## Queues

```python
from queue import Queue

commands: Queue[str] = Queue()
commands.put("stop")
command = commands.get()
```

Queues are safer than casual shared lists for producer-consumer patterns.

## Async IO

```python
import asyncio

async def main() -> None:
    await asyncio.sleep(1)

asyncio.run(main())
```

Async is useful for many waiting tasks, not CPU-heavy computation.

## Exercises

1. Start and join a thread.
2. Use `Queue` to pass commands to a worker.
3. Protect shared state with a lock.
4. Write a small `asyncio` sleep example.
5. Explain GIL implications for CPU-bound robotics workloads.

## Pass Criteria

You pass when you can:

- explain GIL at a practical level;
- choose threads for IO-bound work;
- use queues for communication;
- avoid unsafe shared mutable state;
- explain when multiprocessing or native code is needed.

