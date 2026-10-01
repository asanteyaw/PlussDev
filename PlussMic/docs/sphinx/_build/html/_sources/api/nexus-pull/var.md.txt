<!-- pluss-hand-authored -->

# NexusPull::var

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::var(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds a `var` (variance) aggregate directly on this `NexusPull`, with no
scoping step — `source` must already be a fully qualified `"table.feature"`
path (e.g. `"events.spend"`, meaning table `"events"`, feature `"spend"`).
This is the key difference from [`NexusPullAgg::var`](agg-var.md): calling
`.var(...)` through a scope opened by `.agg("events")` auto-prefixes a bare
feature name with `"events."`, but calling `.var(...)` directly on
`NexusPull` has no scope to inherit a table name from, so the table must be
spelled out in `source` every time.

Internally, this builds a `PullSpec` via the free
[`nexus::var(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism `NexusPullAgg::var` uses once it has prefixed `source` itself.
Reach for this unscoped form for a single reducer on a related table; reach
for `.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to compute the variance of. Typically `"table.feature"` for
    a related table (e.g. `"events.spend"`); a bare root feature name is
    accepted syntactically but rarely useful, per
    [`Nexus::pull`](../nexus/pull.md)'s restriction against aggregate fields
    targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_var` (dots replaced by underscores) — see
    [`NexusPull::sum`](sum.md) for the auto-alias pattern in detail.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped `var`, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .var("events.spend", "var_spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id var_spend
```

Customer 1's group is `{10, 5}`, customer 2's is `{2, 8, 4}`, and customer 3
has no `events` rows at all — following the same rule as
[`sum`](sum.md)/[`mean`](mean.md)/etc., `var` over an empty group is `NULL`,
not `0`.

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_var` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .var("events.spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_var
```

## Notes

- `var` on an empty group is `NULL` — the same rule as
  `sum`/`mean`/`median`/`min`/`max`/`std`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `var` is not supported for windowed pulls yet, only ordinary grouped
  ones — see [`Nexus::pull`](../nexus/pull.md)'s windowed-aggregate
  callout. This applies here too, even though the windowed form can only be
  reached via [`add()`](add.md) with a `PullSpec` built through
  `.windowed(...)`.
- `var` is in squared units of `source`; use [`std`](std.md) for a result in
  the original units.

## See Also

- [`NexusPullAgg::var`](agg-var.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::std`](std.md): the square root of this variance.
- [`NexusPull::mean`](mean.md): the central value `var`/`std` measure spread
  around.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="../../nexus_pull_api_reference.html">
    <span class="label">Next</span>
    <span class="title">NexusPull API Reference</span>
  </a>
</div>
