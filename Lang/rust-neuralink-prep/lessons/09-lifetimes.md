# Lesson 9: Lifetimes

## Goal

Understand how Rust tracks reference validity.

## What Lifetimes Mean

A lifetime is the region where a reference is valid.

Most lifetimes are inferred. You write explicit lifetime annotations when the compiler needs help understanding relationships between references.

## Dangling Prevention

Rust rejects returning references to local values:

```rust
fn bad() -> &String {
    let s = String::from("axis");
    &s
}
```

This cannot compile because `s` is destroyed at function end.

## Lifetime Annotation Example

```rust
fn longer<'a>(left: &'a str, right: &'a str) -> &'a str {
    if left.len() >= right.len() {
        left
    } else {
        right
    }
}
```

The annotation says the returned reference is valid as long as both inputs are valid for the shared lifetime.

## Structs Holding References

```rust
struct NamedAxis<'a> {
    name: &'a str,
}
```

Prefer owned data when it makes the design simpler.

## Exercises

1. Trigger a dangling reference compiler error.
2. Fix it by returning an owned `String`.
3. Write the `longer` function.
4. Create a struct that holds a string reference.
5. Explain when owned data is simpler than lifetime-heavy references.

## Pass Criteria

You pass when you can:

- explain lifetime as reference validity;
- understand common lifetime errors;
- write a simple lifetime annotation;
- avoid unnecessary reference-heavy designs;
- explain how lifetimes support memory safety.

