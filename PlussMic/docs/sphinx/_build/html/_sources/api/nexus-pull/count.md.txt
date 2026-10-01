<!-- pluss-hand-authored -->

# NexusPull::count

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::count(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds a `count` aggregate directly on this `NexusPull`, with no scoping step —
`source` must already be a fully qualified `"table.feature"` path (or, for a
root-self aggregate that would otherwise throw, see
[`Nexus::pull`](../nexus/pull.md)'s Example 8 restriction, still enforced
here). This is the important difference from
[`NexusPullAgg::count`](agg-count.md): the scoped sub-builder returned by
`.agg("events")` auto-prefixes a bare feature name with `"events."` for
every reducer inside the scope, but calling `.count(...)` straight on
`NexusPull` has no scope to inherit a table name from, so the table must be
spelled out in `source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::count(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the exact
same mechanism `NexusPullAgg::count` uses once it has prefixed `source`
itself. Reach for the unscoped form when a single reducer is the only thing
you need from a related table; reach for `.agg(table)` when you want several
reducers on the same table, since opening and closing a scope for just one
call is more ceremony than it buys.

## Parameters

**source** (`std::string_view`)
    The feature to count non-null occurrences of. Typically
    `"table.feature"` for a related table (e.g. `"events.event_id"`); a bare
    root feature name is also accepted syntactically but aggregating a root
    feature against itself is rarely useful and matches
    [`Nexus::pull`](../nexus/pull.md)'s own restriction against targeting
    `root`.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_count` (dots replaced by underscores) — see
    [`Nexus::pull`](../nexus/pull.md)'s "Every output column is aliased
    automatically" rule, which applies identically here.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped count, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .count("events.event_id", "event_count")
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
[`NexusPull::sum`](sum.md)).

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_count` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .count("events.event_id")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_event_id_count
```

## Notes

- `count` here follows `DataFrame`'s own `count` reduction: it counts
  non-null values of `source`, not rows. For a feature that is never null on
  the related table (like an id column), that is the same thing.
- Unlike `sum`/`mean`/`median`/`min`/`max`/`std`/`var`, `count` (and
  [`ndistinct`](ndistinct.md)) never produce `NULL` for an empty group —
  they naturally produce `0`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::count`](agg-count.md).

## See Also

- [`NexusPullAgg::count`](agg-count.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::agg`](agg.md): opens that scope.
- [`NexusPull::ndistinct`](ndistinct.md): count of *distinct* non-null
  values instead of all of them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collect.html">
    <span class="label">Previous</span>
    <span class="title">collect</span>
  </a>
  <a class="next-link" href="end.html">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>
