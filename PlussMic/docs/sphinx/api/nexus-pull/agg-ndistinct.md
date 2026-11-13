<!-- pluss-hand-authored -->

# NexusPullAgg::ndistinct

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::ndistinct(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::ndistinct(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::ndistinct(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds an `ndistinct` aggregate over the table this `NexusPullAgg` is scoped
to — the number of *distinct* non-null values of `feature` per root row —
equivalent to calling [`NexusPull::ndistinct`](ndistinct.md) with `feature`
already prefixed by `"<table>."`. The batch overloads add several
`ndistinct` aggregates on the same scoped table in one call.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own feature to count distinct non-null values of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `ndistinct` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped `ndistinct`, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").ndistinct("event_id", "distinct_event_id").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  distinct_event_id
--------------------------------------
0    | 1            2
1    | 2            3
2    | 3            0
--------------------------------------
[3 cs x 2 feat]
```

Every `event_id` in the sample dataset is unique to its row, so
`ndistinct("event_id", ...)` produces the same counts as
[`count("event_id", ...)`](agg-count.md) here — `2`, `3`, `0`. The two
reducers diverge once a feature repeats within a group (for example,
`ndistinct("customer_id", ...)` inside this same scope would still be `1`
for every non-empty group, while `count` would match the row count).

Example 2. A brace-literal batch of two `ndistinct` requests scoped to the
same table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .ndistinct({{"event_id", "distinct_event_id"},
                    {"customer_id", "distinct_customer_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id distinct_event_id distinct_customer_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"event_id", "distinct_event_id"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").ndistinct(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id distinct_event_id
```

## Notes

- `ndistinct` counts *distinct* non-null values of `feature`, unlike
  [`count`](agg-count.md), which counts *all* non-null values (with
  repeats).
- Unlike `sum`/`mean`/`median`/`min`/`max`/`std`/`var`, `ndistinct` (and
  `count`) never produce `NULL` for an empty group — they naturally produce
  `0`.

## See Also

- [`NexusPull::ndistinct`](ndistinct.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::count`](agg-count.md): count of *all* non-null values
  instead of distinct ones.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-min.html">
    <span class="label">Previous</span>
    <span class="title">agg-min</span>
  </a>
  <a class="next-link" href="agg-std.html">
    <span class="label">Next</span>
    <span class="title">agg-std</span>
  </a>
</div>
