<!-- pluss-hand-authored -->

# NexusPullAgg::min

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg::min(std::string_view feature, std::string_view alias = {}) -> NexusPullAgg&
NexusPullAgg::min(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
NexusPullAgg::min(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
```

## What It Does

Adds a `min` aggregate over the table this `NexusPullAgg` is scoped to — the
smallest value of `feature` per root row — equivalent to calling
[`NexusPull::min`](min.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `min` aggregates on the same
scoped table in one call.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own feature to take the minimum of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `min` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped min, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").min("spend", "min_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  min_spend
------------------------------
0    | 1            5
1    | 2            2
2    | 3            NULL
------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows — `min` over an empty group is `NULL`, not a
sentinel like `0` or `+inf`.

Example 2. A brace-literal batch of two `min` requests scoped to the same
table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .min({{"spend", "min_spend"}, {"event_id", "min_event_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id min_spend min_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "min_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").min(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id min_spend
```

## Notes

- `min` on an empty group is `NULL` — the same rule as `sum`/`mean`/
  `median`/`max`/`std`/`var`, and unlike `count`/`ndistinct`, which produce
  `0`.
- `min` on a text/date feature compares by that feature's own ordering, the
  same as `DataFrame`'s own grouped `min`.

## See Also

- [`NexusPull::min`](min.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::max`](agg-max.md): the complementary reducer.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-median.html">
    <span class="label">Previous</span>
    <span class="title">agg-median</span>
  </a>
  <a class="next-link" href="agg-ndistinct.html">
    <span class="label">Next</span>
    <span class="title">agg-ndistinct</span>
  </a>
</div>
