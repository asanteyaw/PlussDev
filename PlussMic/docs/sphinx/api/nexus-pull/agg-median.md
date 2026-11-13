<!-- pluss-hand-authored -->

# NexusPullAgg::median

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPullAgg& NexusPullAgg::median(std::string_view feature, std::string_view alias = {})
NexusPullAgg& NexusPullAgg::median(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> fields)
NexusPullAgg& NexusPullAgg::median(std::initializer_list<std::pair<std::string_view, std::string_view>> fields)
```

## What It Does

Adds a `median` aggregate over the table this `NexusPullAgg` is scoped to —
the middle value of `feature` per root row — equivalent to calling
[`NexusPull::median`](median.md) with `feature` already prefixed by
`"<table>."`. The batch overloads add several `median` aggregates on the
same scoped table in one call.

`median` is a robust alternative to [`mean`](agg-mean.md): a single unusually
large or small `spend` value shifts a group's mean but has little effect on
its median, since the median only depends on the middle-ranked value(s).

## Parameters

**feature** (`std::string_view`)
    The scoped table's own numeric feature to take the median of.

**alias** (`std::string_view`, default empty)
    The output column name.

**fields** (a range of `(feature, alias)` pairs)
    Several `median` requests on the same scoped table, added in order.

## Returns

`NexusPullAgg&`
    A reference to the same sub-builder, so calls chain in place.

## Examples

Example 1. A single scoped median, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").median("spend", "median_spend").end()
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  median_spend
---------------------------------
0    | 1            7.5
1    | 2            4
2    | 3            NULL
---------------------------------
[3 cs x 2 feat]
```

Customer 1's two events (`10`, `5`) average the two middle values to `7.5`;
customer 2's three events (`2`, `8`, `4`) sort to `2, 4, 8`, so the median is
the single middle value, `4`. Customer 3 has no `events` rows — `median`
over an empty group is `NULL`, the same rule as `sum`/`mean`/`min`/`max`.

Example 2. A brace-literal batch of two medians scoped to the same table.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events")
        .median({{"spend", "median_spend"}, {"event_id", "median_event_id"}})
    .end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id median_spend median_event_id
```

Example 3. The same batch via a named `std::vector` of pairs, exercising the
`RangeView` overload without naming it.

```cpp
std::vector<std::pair<std::string_view, std::string_view>> fields = {
    {"spend", "median_spend"}};
auto pulled = nx.pull("customers")
    .take("customer_id")
    .agg("events").median(fields).end()
    .collect();
std::cout << pulled.features();
```

### Output

```text
customer_id median_spend
```

## Notes

- `median` on an empty group is `NULL`, the same rule as `sum`/`mean`/`min`/
  `max`/`std`/`var`.
- For an even-sized group, `median` averages the two middle-ranked values
  (customer 1's `{10, 5}` above), the same convention `DataFrame`'s own
  grouped `median` uses.
- For a windowed median, build the spec via [`add()`](add.md) with the free
  [`nexus::median(...)`](../nexus/pull.md) factory and `.windowed(...)` —
  the fluent `.median(...)` calls on this page always aggregate the *entire*
  related group, unwindowed.

## See Also

- [`NexusPull::median`](median.md): the unscoped equivalent, taking an
  already-qualified `"table.feature"` source.
- [`NexusPullAgg::mean`](agg-mean.md): the non-robust counterpart.
- [`NexusPullAgg::end`](end.md): return to the parent `NexusPull`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="agg-mean.html">
    <span class="label">Previous</span>
    <span class="title">agg-mean</span>
  </a>
  <a class="next-link" href="agg-min.html">
    <span class="label">Next</span>
    <span class="title">agg-min</span>
  </a>
</div>
