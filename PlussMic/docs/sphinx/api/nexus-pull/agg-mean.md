<!-- pluss-hand-authored -->

# NexusPullAgg::mean

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::mean(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::mean(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::mean(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds a `mean` aggregate over the table this `NexusPullAgg` is scoped to —
the average of `feature` per root row — equivalent to calling
[`NexusPull::mean`](mean.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `mean` aggregates on the same
scoped table in one call.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own numeric feature to average.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `mean` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped mean, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").mean("spend", "mean_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  mean_spend
-------------------------------
0    | 1            7.5
1    | 2            4.666666667
2    | 3            NULL
-------------------------------
[3 cs x 2 feat]
```

Example 2. A brace-literal batch of two means scoped to the same table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .mean({{"spend", "mean_spend"}, {"event_id", "mean_event_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id mean_spend mean_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "mean_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").mean(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id mean_spend
```

## Notes

- `mean` on an empty group is `NULL`, the same rule as `sum`/`median`/`min`/
  `max`/`std`/`var`.
- For a windowed mean, build the spec via [`add()`](add.md) with the free
  [`nexus::mean(...)`](../nexus/pull.md) factory and `.windowed(...)` — the
  fluent `.mean(...)` calls on this page always aggregate the *entire*
  related group, unwindowed.

## See Also

- [`NexusPull::mean`](mean.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::median`](agg-median.md): a robust alternative to `mean`.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-max.html">
    <span class="label">Previous</span>
    <span class="title">agg-max</span>
  </a>
  <a class="next-link" href="agg-median.html">
    <span class="label">Next</span>
    <span class="title">agg-median</span>
  </a>
</div>
