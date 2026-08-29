# `lymph::prep::route::where`

## Signature

```cpp
lymph::prep::Route lymph::prep::route::where(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);
```

## What It Does

`where(...)` builds a route from a predicate instead of a fixed name list.

Use it when the route should follow a rule such as:

- every numeric feature
- every note-like feature
- every name with a shared prefix

This is more dynamic than `names(...)`, but that flexibility comes with a
burden: the reader now has to understand the predicate to understand the
claimed features.

## Practical Example

```{literalinclude} ../../../../../../examples/NodeJ/lymph/lymph_prep_column_router_docverify_demo.cpp
:language: cpp
```

Output

```text
== names ==
named = [0, 1]

== where_and_remainder ==
numeric = [0, 1, 2]
categorical = [3]
remainder = [0, 1, 2]
```

## Related Pages

- [names](names.md)
- [remaining_of](remaining_of.md)
- [Route](Route.md)
