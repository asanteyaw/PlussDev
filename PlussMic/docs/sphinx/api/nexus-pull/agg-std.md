<!-- pluss-hand-authored -->

# NexusPullAgg::std

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::std(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::std(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::std(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds a `std` aggregate over the table this `NexusPullAgg` is scoped to — the
standard deviation of `feature` per root row, using the same convention as
`DataFrame`'s own grouped `std` — equivalent to calling
[`NexusPull::std`](std.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `std` aggregates on the same
scoped table in one call.

This page does not commit to whether that convention is the sample or
population standard deviation — see [`DataFrame`'s own `std`
reference](../dataframe/std.md) for that detail. The examples below print
only column names, not computed values, so nothing here depends on it.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own numeric feature to take the standard deviation
    of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `std` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped `std`, with an explicit alias, alongside `sum`
and `count` in the same `.agg(...)` scope.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .count("event_id", "event_count")
        .sum("spend", "total_spend")
        .std("spend", "std_spend")
    .end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id event_count total_spend std_spend
```

Customer 1's group has two `spend` values and customer 2's has three, so
`std_spend` is well defined for both; customer 3's group is empty, so its
`std_spend` is `NULL` — the same empty-group rule `sum`/`mean`/`median`/
`min`/`max` follow (see [Notes](#notes) below).

Example 2. A brace-literal batch of two `std` requests scoped to the same
table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .std({{"spend", "std_spend"}, {"event_id", "std_event_id"}})
    .end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id std_spend std_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "std_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").std(fields).end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id std_spend
```

## Notes

- `std` on an empty group is `NULL`, the same rule as `sum`/`mean`/
  `median`/`min`/`max`/`var`, and unlike `count`/`ndistinct`, which produce
  `0`.
- `std` on a group of exactly one value is also a degenerate case worth
  checking against [`DataFrame`'s own grouped `std`](../dataframe/std.md)
  convention before relying on its value in downstream code.
- `std` and [`var`](agg-var.md) are related the same way they are on
  `DataFrame`: `std` is the square root of `var`, computed under the same
  convention.

## See Also

- [`NexusPull::std`](std.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::var`](agg-var.md): the squared counterpart.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">agg-ndistinct</span>
  </a>
  <a class="next-link" href="agg-sum.html">
    <span class="label">Next</span>
    <span class="title">agg-sum</span>
  </a>
</div>
