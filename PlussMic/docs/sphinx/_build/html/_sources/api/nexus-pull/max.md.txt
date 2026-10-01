<!-- pluss-hand-authored -->

# NexusPull::max

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::max(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds a `max` aggregate directly on this `NexusPull`, with no scoping step —
`source` must already be a fully qualified `"table.feature"` path (e.g.
`"events.spend"`, meaning table `"events"`, feature `"spend"`). This is the
key difference from [`NexusPullAgg::max`](agg-max.md): calling `.max(...)`
through a scope opened by `.agg("events")` auto-prefixes a bare feature name
with `"events."`, but calling `.max(...)` directly on `NexusPull` has no
scope to inherit a table name from, so the table must be spelled out in
`source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::max(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism `NexusPullAgg::max` uses once it has prefixed `source` itself.
Reach for this unscoped form for a single reducer on a related table; reach
for `.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to take the maximum of. Typically `"table.feature"` for a
    related table (e.g. `"events.spend"`); a bare root feature name is
    accepted syntactically but rarely useful, per
    [`Nexus::pull`](../nexus/pull.md)'s restriction against aggregate fields
    targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_max` (dots replaced by underscores) — see
    [`NexusPull::sum`](sum.md) for the auto-alias pattern in detail.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped max, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .max("events.spend", "max_spend")
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

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_max` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .max("events.spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_max
```

## Notes

- `max` on an empty group is `NULL` — the same rule as
  `sum`/`mean`/`median`/`min`/`std`/`var`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `max` on a text/date feature compares by that feature's own ordering, the
  same as `DataFrame`'s own grouped `max`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::max`](agg-max.md).

## See Also

- [`NexusPullAgg::max`](agg-max.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::min`](min.md): the complementary reducer.
- [`NexusPull::agg`](agg.md): opens the scoped sub-builder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="end.html">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
