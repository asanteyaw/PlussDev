<!-- pluss-hand-authored -->

# NexusPullAgg::count

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::count(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::count(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::count(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds a `count` aggregate over the table this `NexusPullAgg` is scoped to —
the number of related rows per root row — equivalent to calling
[`NexusPull::count`](count.md) with `feature` already prefixed by
`"<table>."`. This is what `.agg(table)` buys over the unscoped reducers:
write the table name once, then `.count(...)`, `.sum(...)`, and the rest all
inherit it.

The batch overloads (`RangeView`/`initializer_list` of `(feature, alias)`
pairs) add several `count` aggregates on the same scoped table in one call,
equivalent to calling the scalar overload once per pair.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own feature to count non-null occurrences of.

**alias** (`std::string_view`, default empty)
    The output column name. Aggregate reducers generally want an explicit
    alias — see [`Nexus::pull`](../nexus/pull.md) for the default
    `<table>_<feature>_<aggregate>` naming when one is omitted.

**fields** (a range of `(feature, alias)` pairs)
    Several `count` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped count, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").count("event_id", "event_count").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  event_count
--------------------------------
0    | 1            2
1    | 2            3
2    | 3            0
--------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows at all — `count` over an empty group is `0`,
unlike a `sum`/`mean`/etc. over the same empty group, which is `NULL` (see
[`NexusPullAgg::sum`](agg-sum.md)).

Example 2. A brace-literal batch of two counts scoped to the same table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .count({{"event_id", "event_count"}, {"customer_id", "row_count"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id event_count row_count
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"event_id", "event_count"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").count(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id event_count
```

## Notes

- `count` here follows `DataFrame`'s own `count` reduction: it counts
  non-null values of `feature`, not rows. For a feature that is never null
  on the related table (like an id column), that is the same thing.
- Unlike `sum`/`mean`/`median`/`min`/`max`/`std`/`var`, `count` (and
  `ndistinct`) never produce `NULL` for an empty group — they naturally
  produce `0`.

## See Also

- [`NexusPull::count`](count.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::ndistinct`](agg-ndistinct.md): count of *distinct*
  non-null values instead of all of them.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg.html">
    <span class="label">Previous</span>
    <span class="title">agg</span>
  </a>
  <a class="next-link" href="agg-max.html">
    <span class="label">Next</span>
    <span class="title">agg-max</span>
  </a>
</div>
