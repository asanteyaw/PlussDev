# `lymph::prep::route::remaining_of`

## Signature

```cpp
lymph::prep::Route lymph::prep::route::remaining_of(std::vector<std::string> names);
lymph::prep::Route lymph::prep::route::remaining_of(lymph::prep::Route other);
```

## What It Does

`remaining_of(...)` builds the complement of another claimed feature set.

Use the name-list overload when the claimed set is simple and static.

Use the route overload when the remainder should stay tied to another route’s
actual logic. That second form is the safer one when the primary route is
predicate-driven, because it prevents a hand-maintained complement list from
drifting out of sync.

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
- [where](where.md)
- [Route](Route.md)
