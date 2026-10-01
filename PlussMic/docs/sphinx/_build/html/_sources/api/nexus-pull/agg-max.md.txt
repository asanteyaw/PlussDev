<!-- pluss-hand-authored -->

# NexusPullAgg::max

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg::max(std::string_view feature, std::string_view alias = {}) -> NexusPullAgg&
NexusPullAgg::max(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
NexusPullAgg::max(std::initializer_list<std::pair<std::string_view, std::string_view>> fields) -> NexusPullAgg&
```

## What It Does

Adds a `max` aggregate over the table this `NexusPullAgg` is scoped to — the
largest value of `feature` per root row — equivalent to calling
[`NexusPull::max`](max.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `max` aggregates on the same
scoped table in one call.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own feature to take the maximum of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `max` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped max, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").max("spend", "max_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  max_spend
------------------------------
0    | 1            10
1    | 2            8
2    | 3            NULL
------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows — `max` over an empty group is `NULL`, not a
sentinel like `0` or `-inf`.

Example 2. A brace-literal batch of two `max` requests scoped to the same
table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .max({{"spend", "max_spend"}, {"event_id", "max_event_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id max_spend max_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "max_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").max(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id max_spend
```

## Notes

- `max` on an empty group is `NULL` — the same rule as `sum`/`mean`/`median`/
  `min`/`std`/`var`, and unlike `count`/`ndistinct`, which produce `0`.
- `max` on a text/date feature compares by that feature's own ordering, the
  same as `DataFrame`'s own grouped `max`.

## See Also

- [`NexusPull::max`](max.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::min`](agg-min.md): the complementary reducer.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-count.html">
    <span class="label">Previous</span>
    <span class="title">agg-count</span>
  </a>
  <a class="next-link" href="agg-mean.html">
    <span class="label">Next</span>
    <span class="title">agg-mean</span>
  </a>
</div>
