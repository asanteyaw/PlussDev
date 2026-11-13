<!-- pluss-hand-authored -->

# NexusPull::sum

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull& NexusPull::sum(std::string_view source, std::string_view alias = {})
```

## What It Does

Adds a `sum` aggregate directly on this `NexusPull`, with no scoping step —
`source` must already be a fully qualified `"table.feature"` path (e.g.
`"events.spend"`, meaning table `"events"`, feature `"spend"`). This is the
key difference from [`NexusPullAgg::sum`](agg-sum.md): calling `.sum(...)`
through a scope opened by `.agg("events")` auto-prefixes a bare feature name
with `"events."`, but calling `.sum(...)` directly on `NexusPull` has no
scope to inherit a table name from, so the table must be spelled out in
`source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::sum(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the exact
same mechanism `NexusPullAgg::sum` uses once it has prefixed `source`
itself. These unscoped reducers exist for the common case of one reducer on
a related table, where opening and closing an `.agg(table)...end()` scope
for a single call would be more ceremony than it's worth; `NexusPullAgg`
earns its keep when you want several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to sum. Typically `"table.feature"` for a related table
    (e.g. `"events.spend"`); a bare root feature name is accepted
    syntactically but rarely useful, per [`Nexus::pull`](../nexus/pull.md)'s
    restriction against aggregate fields targeting `root` itself. An empty
    `source` throws — see Example 2.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_sum` (dots replaced by underscores) — see
    [`Nexus::pull`](../nexus/pull.md)'s "Every output column is aliased
    automatically" rule, and Example 3 below.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped sum, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .sum("events.spend", "total_spend")
    .collect();

std::cout << pulled.to_string();
```

### Output

```text
     | customer_id  total_spend
---------------------------------
0    | 1            15
1    | 2            14
2    | 3            NULL
---------------------------------
[3 cs x 2 feat]
```

Customer 3 has no `events` rows at all — `sum` over an empty group is
`NULL`, not `0` (contrast [`count`](count.md), where the empty group
produces `0`).

Example 2. An empty `source` throws.

```cpp
try {
  auto bad = nx.pull("customers").sum("", "bad");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
pull spec source cannot be empty
```

Example 3. Omitting `alias` falls back to the default
`<table>_<feature>_sum` naming.

```cpp
auto pulled = nx.pull("customers").take("customer_id").sum("events.spend").collect();
std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_sum
```

The default alias pattern — `<table>_<feature>_<aggregate>` with dots
replaced by underscores — is the same one [`Nexus::pull`](../nexus/pull.md)
documents for the string-grammar form, and it applies identically to every
other unscoped reducer on this page ([`mean`](mean.md), [`median`](median.md),
[`min`](min.md), [`max`](max.md), [`std`](std.md), [`var`](var.md),
[`count`](count.md), [`ndistinct`](ndistinct.md)).

## Notes

- `sum` on an empty group is `NULL` — the same rule as
  `mean`/`median`/`min`/`max`/`std`/`var`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::sum`](agg-sum.md).

## See Also

- [`NexusPullAgg::sum`](agg-sum.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::mean`](mean.md): the related average, still `NULL` on an
  empty group.
- [`NexusPull::agg`](agg.md): opens the scoped sub-builder.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
