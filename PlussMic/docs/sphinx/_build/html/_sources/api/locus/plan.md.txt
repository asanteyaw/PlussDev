<!-- pluss-hand-authored -->

# Locus::plan

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::plan(std::string_view from, std::string_view to, JoinHow how = JoinHow::Left) const -> JoinPlan
Locus::plan(const JoinPath& path, JoinHow how = JoinHow::Left) const -> JoinPlan
```

## What It Does

Builds a `JoinPlan` -- the shortest chain of links between two catalogs, as
a concrete sequence of join steps -- without executing anything. The
`(from, to)` overload finds the path itself (the same shortest-path search
[`path`](link.md) exposes); the `JoinPath` overload builds a plan directly
from a path already found. Neither call joins any data; see
[`materialize`](materialize.md) to actually run the plan, or
[`store`](store.md) to save it under a name for repeated use.

## Parameters

| Parameter | Meaning |
|---|---|
| `from`, `to` | The two catalogs to connect. |
| `path` | A `JoinPath` (from [`path`](link.md)) to build a plan from directly. |
| `how` | The join kind applied at every hop. Default `Left`. |

## Returns

A `JoinPlan`: a `root_table` and an ordered list of `JoinPlanStep`s.

## Examples

Example 1. The shortest plan between two catalogs two hops apart.

```cpp
const auto linked = make_linked_shop();
const auto shortest = linked.plan("items", "customers", JoinHow::Left);
std::cout << "plan steps=" << shortest.steps.size() << " root_table=" << shortest.root_table << "\n";
```

### Output

```text
plan steps=2 root_table=items
```

## See Also

- [`materialize`](materialize.md): actually running a plan.
- [`store`](store.md): saving a plan under a name to run again later.
- [`link`](link.md): `path`, the shortest-path search this builds a plan
  from.
