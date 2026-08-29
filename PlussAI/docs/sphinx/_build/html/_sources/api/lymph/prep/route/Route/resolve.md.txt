# `lymph::prep::Route::resolve`

## Signature

```cpp
std::vector<std::int64_t> lymph::prep::Route::resolve(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& ordered_names) const;

std::vector<std::int64_t> lymph::prep::Route::resolve(
    const pluss::table::DataFrame& frame) const;
```

## What It Does

`resolve(...)` turns a declarative route into concrete feature positions.

The two overloads differ in how explicit the feature order is:

- `resolve(frame, ordered_names)` is the fully explicit and safer form
- `resolve(frame)` uses the frame’s own visible feature order

The explicit overload matters whenever note features or non-default batch
options influence the effective feature order seen by later preprocessing.

## Practical Example

```{literalinclude} ../../../../../../../examples/NodeJ/lymph/lymph_prep_column_router_docverify_demo.cpp
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

- [with_remainder](with_remainder.md)
- [Notes](Notes.md)
