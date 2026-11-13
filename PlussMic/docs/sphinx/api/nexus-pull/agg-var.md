<!-- pluss-hand-authored -->

# NexusPullAgg::var

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::var(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::var(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::var(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds a `var` aggregate over the table this `NexusPullAgg` is scoped to — the
variance of `feature` per root row, using the same convention as
`DataFrame`'s own grouped `var` — equivalent to calling
[`NexusPull::var`](var.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `var` aggregates on the same
scoped table in one call.

This page does not commit to whether that convention is the sample or
population variance — see [`DataFrame`'s own `var`
reference](../dataframe/var.md) for that detail. The examples below print
only column names, not computed values, so nothing here depends on it.

## Parameters

**feature** (`std::string_view`)
    The scoped table's own numeric feature to take the variance of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `var` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped `var`, with an explicit alias, alongside `sum`
and `count` in the same `.agg(...)` scope.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .count("event_id", "event_count")
        .sum("spend", "total_spend")
        .var("spend", "var_spend")
    .end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id event_count total_spend var_spend
```

Customer 1's group has two `spend` values and customer 2's has three, so
`var_spend` is well defined for both; customer 3's group is empty, so its
`var_spend` is `NULL` — the same empty-group rule `sum`/`mean`/`median`/
`min`/`max` follow (see [Notes](#notes) below).

Example 2. A brace-literal batch of two `var` requests scoped to the same
table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .var({{"spend", "var_spend"}, {"event_id", "var_event_id"}})
    .end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id var_spend var_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "var_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").var(fields).end()
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id var_spend
```

## Notes

- `var` on an empty group is `NULL`, the same rule as `sum`/`mean`/
  `median`/`min`/`max`/`std`, and unlike `count`/`ndistinct`, which produce
  `0`.
- `var` on a group of exactly one value is also a degenerate case worth
  checking against [`DataFrame`'s own grouped `var`](../dataframe/var.md)
  convention before relying on its value in downstream code.
- `var` and [`std`](agg-std.md) are related the same way they are on
  `DataFrame`: `var` is `std` squared, computed under the same convention.

## See Also

- [`NexusPull::var`](var.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::std`](agg-std.md): the square-rooted counterpart.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-sum.html">
    <span class="label">Previous</span>
    <span class="title">agg-sum</span>
  </a>
  <a class="next-link" href="collect.html">
    <span class="label">Next</span>
    <span class="title">collect</span>
  </a>
</div>
