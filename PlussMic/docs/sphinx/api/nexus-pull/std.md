<!-- pluss-hand-authored -->

# NexusPull::std

[Back to NexusPull API Reference](../../nexus_pull_api_reference)

## Signature

```cpp
NexusPull::std(std::string_view source, std::string_view alias = {}) -> NexusPull&
```

## What It Does

Adds a `std` (standard deviation) aggregate directly on this `NexusPull`,
with no scoping step — `source` must already be a fully qualified
`"table.feature"` path (e.g. `"events.spend"`, meaning table `"events"`,
feature `"spend"`). This is the key difference from
[`NexusPullAgg::std`](agg-std.md): calling `.std(...)` through a scope
opened by `.agg("events")` auto-prefixes a bare feature name with
`"events."`, but calling `.std(...)` directly on `NexusPull` has no scope to
inherit a table name from, so the table must be spelled out in `source`
every time.

Internally, this builds a `PullSpec` via the free
[`nexus::std(source, alias)`](take.md) factory (in namespace
`pluss::bundle::nexus`) and appends it with [`add()`](add.md) — the same
mechanism `NexusPullAgg::std` uses once it has prefixed `source` itself.
Reach for this unscoped form for a single reducer on a related table; reach
for `.agg(table)` when stacking several reducers on the same table.

## Parameters

**source** (`std::string_view`)
    The feature to compute the standard deviation of. Typically
    `"table.feature"` for a related table (e.g. `"events.spend"`); a bare
    root feature name is accepted syntactically but rarely useful, per
    [`Nexus::pull`](../nexus/pull.md)'s restriction against aggregate fields
    targeting `root` itself.

**alias** (`std::string_view`, default empty)
    The output column name. When omitted, the default alias is
    `<table>_<feature>_std` (dots replaced by underscores) — see
    [`NexusPull::sum`](sum.md) for the auto-alias pattern in detail.

## Returns

`NexusPull&`
    A reference to the same builder, so calls chain in place.

## Examples

Example 1. A single unscoped `std`, with an explicit alias.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto pulled = nx.pull("customers")
    .take("customer_id")
    .std("events.spend", "std_spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id std_spend
```

Customer 1's group is `{10, 5}`, customer 2's is `{2, 8, 4}`, and customer 3
has no `events` rows at all — following the same rule as
[`sum`](sum.md)/[`mean`](mean.md)/etc., `std` over an empty group is `NULL`,
not `0`.

Example 2. Omitting `alias` falls back to the default
`<table>_<feature>_std` naming.

```cpp
auto pulled = nx.pull("customers")
    .take("customer_id")
    .std("events.spend")
    .collect();

std::cout << pulled.features();
```

### Output

```text
customer_id events_spend_std
```

## Notes

- `std` on an empty group is `NULL` — the same rule as
  `sum`/`mean`/`median`/`min`/`max`/`var`, and unlike `count`/`ndistinct`,
  which produce `0`.
- `std` is not supported for windowed pulls yet, only ordinary grouped ones —
  see [`Nexus::pull`](../nexus/pull.md)'s windowed-aggregate callout. This
  applies here too, even though the windowed form can only be reached via
  [`add()`](add.md) with a `PullSpec` built through `.windowed(...)`.
- `std` is the square root of [`var`](var.md) — the two follow the same
  sample/population convention as the rest of the aggregate surface.
- `source` is used verbatim — there is no table-name prefixing here, unlike
  [`NexusPullAgg::std`](agg-std.md).

## See Also

- [`NexusPullAgg::std`](agg-std.md): the scoped equivalent, for when you
  want several reducers on the same related table without repeating its
  name.
- [`NexusPull::var`](var.md): the squared-units counterpart `std` is the
  square root of.
- [`NexusPull::mean`](mean.md): the central value `std`/`var` measure spread
  around.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="specs.html">
    <span class="label">Previous</span>
    <span class="title">specs</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
