<!-- pluss-hand-authored -->

# NexusPull::ndistinct

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::ndistinct(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds an `ndistinct` aggregate directly on this `NexusPull`, with no scoping
step — `source` must already be a fully qualified `"table.feature"` path.
As with every unscoped reducer on `NexusPull`, this is the key difference
from [`NexusPullAgg::ndistinct`](agg-ndistinct.md): the scoped sub-builder
returned by `.agg("events")` auto-prefixes a bare feature name with
`"events."`, but there is no scope here to inherit a table name from, so
`source` must spell out the table itself.

Internally, this builds a `PullSpec` via the free
[`nexus::ndistinct(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism every reducer on `NexusPull`/`NexusPullAgg` bottoms out in. Prefer
this unscoped form for a single reducer on a related table; prefer
`.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to count distinct non-null values of. Typically
    `"table.feature"` for a related table (e.g. `"events.event_id"`); a bare
    root feature name is accepted syntactically but rarely useful, per
    [`Nexus::pull`](../nexus/pull.md)'s restriction against aggregate fields
    targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_ndistinct` (dots replaced by underscores) — see
    [`Nexus::pull`](../nexus/pull.md)'s automatic aliasing rule.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped `ndistinct`, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .ndistinct("events.event_id", "distinct_events")
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  distinct_events
------------------------------------
0    | 1            2
1    | 2            3
2    | 3            0
------------------------------------
[3 cs x 2 feat]
```

`event_id` is unique per event row here, so `ndistinct` matches
[`count`](count.md)'s own values exactly — customer 3 again gets `0`, not
`NULL`, for the empty group.

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_ndistinct` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .ndistinct("events.event_id")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_event_id_ndistinct
```

## Notes

- `ndistinct` counts distinct non-null values of `source` per root row —
  different from [`count`](count.md), which counts every non-null
  occurrence including duplicates.
- Unlike `sum`/`mean`/`median`/`min`/`max`/`std`/`var`, `ndistinct` (and
  [`count`](count.md)) never produce `NULL` for an empty group — they
  naturally produce `0`.
- `ndistinct` windows are not supported by the underlying pull engine and
  throw `"ndistinct windows are not supported yet"` — see
  [`Nexus::pull`](../nexus/pull.md)'s windowed-aggregate callout. This
  applies here too, even though the windowed form can only be reached via
  [`add()`](add.md) with a `PullSpec` built through `.windowed(...)`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::ndistinct`](agg-ndistinct.md).

## See Also

- [`NexusPullAgg::ndistinct`](agg-ndistinct.md): the scoped equivalent, for
  when you want several reducers on the same related table without
  repeating its name.
- [`NexusPull::count`](count.md): count of all non-null values instead of
  only distinct ones.
- [`NexusPull::agg`](agg.md): opens the scoped sub-builder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="root.html">
    <span class="label">Next</span>
    <span class="title">root</span>
  </a>
</div>
