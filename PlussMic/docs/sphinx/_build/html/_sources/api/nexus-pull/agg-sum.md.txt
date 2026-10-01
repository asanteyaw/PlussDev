<!-- pluss-hand-authored -->

# NexusPullAgg::sum

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg::sum(std::string_view feature, std::string_view alias = {}) -> NexusPullAgg&
NexusPullAgg::sum(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
NexusPullAgg::sum(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
```

## What It Does

Adds a `sum` aggregate over the table this `NexusPullAgg` is scoped to — the
total of `feature` per root row — equivalent to calling
[`NexusPull::sum`](sum.md) with `feature` already prefixed by
`"<table>."`. This is the reducer shown in
[`Nexus::pull`](../nexus/pull.md)'s and [`add()`](add.md)'s own examples.
The batch overloads add several `sum` aggregates on the same scoped table in
one call.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own numeric feature to total.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `sum` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped sum, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").sum("spend", "total_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  total_spend
--------------------------------
0    | 1            15
1    | 2            14
2    | 3            NULL
--------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows — `sum` over an empty group is `NULL`, not
`0` — see [`NexusPullAgg::count`](agg-count.md) for the reducers that use `0`
instead.

Example 2. A brace-literal batch of two sums scoped to the same table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .sum({{"spend", "total_spend"}, {"event_id", "sum_event_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id total_spend sum_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "total_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").sum(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id total_spend
```

## Notes

- `sum` on an empty group is `NULL`, the same rule as `mean`/`median`/`min`/
  `max`/`std`/`var`, and unlike `count`/`ndistinct`, which produce `0`.
- For a windowed sum, build the spec via [`add()`](add.md) with the free
  [`nexus::sum(...)`](../nexus/pull.md) factory and `.windowed(...)` —
  [`add()`'s own Example 3](add.md) shows exactly this
  (`sum("events.spend", "recent_spend").windowed(30, "d")`). The fluent
  `.sum(...)` calls on this page always aggregate the *entire* related
  group, unwindowed.

## See Also

- [`NexusPull::sum`](sum.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::mean`](agg-mean.md): the same total, divided by the group
  size.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-std.html">
    <span class="label">Previous</span>
    <span class="title">agg-std</span>
  </a>
  <a class="next-link" href="agg-var.html">
    <span class="label">Next</span>
    <span class="title">agg-var</span>
  </a>
</div>
