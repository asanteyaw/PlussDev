<!-- pluss-hand-authored -->

# NexusPull::mean

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull& NexusPull::mean(std::string_view source, std::string_view alias = {})
```

## What It Does

Adds a `mean` aggregate directly on this `NexusPull`, with no scoping step —
`source` must already be a fully qualified `"table.feature"` path (e.g.
`"events.spend"`, meaning table `"events"`, feature `"spend"`). This is the
key difference from [`NexusPullAgg::mean`](agg-mean.md): calling `.mean(...)`
through a scope opened by `.agg("events")` auto-prefixes a bare feature name
with `"events."`, but calling `.mean(...)` directly on `NexusPull` has no
scope to inherit a table name from, so the table must be spelled out in
`source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::mean(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism `NexusPullAgg::mean` uses once it has prefixed `source` itself.
Reach for this unscoped form for a single reducer on a related table;
reach for `.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to average. Typically `"table.feature"` for a related table
    (e.g. `"events.spend"`); a bare root feature name is accepted
    syntactically but rarely useful, per [`Nexus::pull`](../nexus/pull.md)'s
    restriction against aggregate fields targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_mean` (dots replaced by underscores) — see
    [`NexusPull::sum`](sum.md) for the auto-alias pattern in detail.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped mean, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .mean("events.spend", "avg_spend")
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  avg_spend
-------------------------------
0    | 1            7.5
1    | 2            4.666666667
2    | 3            NULL
-------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows at all — `mean` over an empty group is
`NULL`, not `0`.

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_mean` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .mean("events.spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_mean
```

## Notes

- `mean` on an empty group is `NULL` — the same rule as
  `sum`/`median`/`min`/`max`/`std`/`var`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::mean`](agg-mean.md).

## See Also

- [`NexusPullAgg::mean`](agg-mean.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::sum`](sum.md): the unaveraged total behind this mean.
- [`NexusPull::median`](median.md): a middle-value alternative, less
  sensitive to outliers than `mean`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
