<!-- pluss-hand-authored -->

# NexusPull::median

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::median(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds a `median` aggregate directly on this `NexusPull`, with no scoping
step — `source` must already be a fully qualified `"table.feature"` path
(e.g. `"events.spend"`, meaning table `"events"`, feature `"spend"`). This is
the key difference from [`NexusPullAgg::median`](agg-median.md): calling
`.median(...)` through a scope opened by `.agg("events")` auto-prefixes a
bare feature name with `"events."`, but calling `.median(...)` directly on
`NexusPull` has no scope to inherit a table name from, so the table must be
spelled out in `source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::median(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism `NexusPullAgg::median` uses once it has prefixed `source` itself.
Reach for this unscoped form for a single reducer on a related table; reach
for `.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to take the median of. Typically `"table.feature"` for a
    related table (e.g. `"events.spend"`); a bare root feature name is
    accepted syntactically but rarely useful, per
    [`Nexus::pull`](../nexus/pull.md)'s restriction against aggregate fields
    targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_median` (dots replaced by underscores) — see
    [`NexusPull::sum`](sum.md) for the auto-alias pattern in detail.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped median, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .median("events.spend", "median_spend")
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  median_spend
-----------------------------------
0    | 1            7.5
1    | 2            4
2    | 3            NULL
-----------------------------------
[3 cs x 2 feat]
```

Customer 1's group is `{10, 5}` (median of the two is `7.5`), customer 2's is
`{2, 8, 4}` (median `4`), and customer 3 has no `events` rows at all — median
over an empty group is `NULL`, not `0`.

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_median` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .median("events.spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_median
```

## Notes

- `median` on an empty group is `NULL` — the same rule as
  `sum`/`mean`/`min`/`max`/`std`/`var`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `median` is not supported for windowed pulls yet, only ordinary grouped
  ones — see [`Nexus::pull`](../nexus/pull.md)'s windowed-aggregate
  callout. This applies here too, even though the windowed form can only be
  reached via [`add()`](add.md) with a `PullSpec` built through
  `.windowed(...)`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::median`](agg-median.md).

## See Also

- [`NexusPullAgg::median`](agg-median.md): the scoped equivalent, for when
  you want several reducers on the same related table without repeating its
  name.
- [`NexusPull::mean`](mean.md): the average alternative, more sensitive to
  outliers than `median`.
- [`NexusPull::min`](min.md) / [`max`](max.md): the extremes bracketing the
  same distribution.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
