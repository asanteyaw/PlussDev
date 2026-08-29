# `lymph::prep::route::names`

## Signature

```cpp
lymph::prep::Route lymph::prep::route::names(std::vector<std::string> names);
```

## What It Does

`names(...)` builds a route that claims exactly the named features, in the
ordered feature space the route is later resolved against.

Use it when the feature subset is explicit and stable. This is the most common
route constructor in production code because it keeps the contract obvious in
review and easy to compare across experiments.

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

- [where](where.md)
- [remaining_of](remaining_of.md)
- [Route](Route.md)
