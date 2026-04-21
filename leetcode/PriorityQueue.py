"""
Priority Queue Crash Course (Interview Prep)

What it is:
- Data structure where each item has priority.
- Removal returns the highest-priority item (or lowest in a min-heap).

Typical implementation:
- Binary heap (usually array-backed).
- Python's `heapq` is a min-heap.

Big-O with heap:
- Push: O(log n)
- Pop: O(log n)
- Peek: O(1)
- Build heap from list: O(n)

Interview patterns:
- Top K elements
- Merge K sorted lists
- Scheduling / task ordering
- Dijkstra (min-priority queue)
"""


class PriorityQueue:
    def __init__(self):
        self.heap = []

    def push(self, item, priority):
        """Insert (item, priority) into the queue."""
        pass

    def pop(self):
        """Remove and return highest-priority item."""
        pass

    def peek(self):
        """Return highest-priority item without removing it."""
        pass

    def is_empty(self):
        """Return True if queue is empty."""
        pass

    def size(self):
        """Return number of elements in queue."""
        pass

    def build_heap(self, items):
        """Build heap from a list of (item, priority) pairs."""
        pass

    def change_priority(self, item, new_priority):
        """Update priority for an existing item (if supported)."""
        pass

