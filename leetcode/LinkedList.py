"""
Linked List Crash Course (Interview Prep)

What it is:
- A linear data structure made of nodes.
- Each node stores a value and a pointer/reference to the next node.

When to use:
- Frequent insertions/deletions near the head or after a known node.
- You do not need random index access.

Core tradeoffs:
- Pros: O(1) insert/delete at head, dynamic size, easy pointer problems.
- Cons: O(n) search/access by index, extra pointer memory.

Big-O cheat sheet (singly linked list):
- Access by index: O(n)
- Search by value: O(n)
- Insert at head: O(1)
- Insert at tail: O(1) with tail pointer, else O(n)
- Delete head: O(1)

Interview patterns:
- Fast/slow pointers (middle, cycle detection)
- Reverse in-place
- Merge two sorted lists
- Dummy node usage for clean edge-case handling
"""


class ListNode:
    def __init__(self, value=0, next_node=None):
        self.value = value
        self.next = next_node


class SinglyLinkedList:
    def __init__(self):
        self.head = None
        self.tail = None
        self.length = 0

    def append(self, value):
        """Add a node to the end of the list."""
        pass

    def prepend(self, value):
        """Add a node to the start of the list."""
        pass

    def insert_at_index(self, index, value):
        """Insert value before index."""
        pass

    def delete_by_value(self, value):
        """Delete first node containing value."""
        pass

    def delete_at_index(self, index):
        """Delete node at index."""
        pass

    def search(self, value):
        """Return index of value, or -1 if not found."""
        pass

    def reverse(self):
        """Reverse the list in-place."""
        pass

    def find_middle(self):
        """Return middle node/value using fast/slow pointers."""
        pass

    def has_cycle(self):
        """Return True if a cycle exists (Floyd's algorithm)."""
        pass

    def to_list(self):
        """Return a Python list of node values."""
        pass

