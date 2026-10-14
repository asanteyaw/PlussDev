# Bundle Model Guide

## When To Use Bundle Types

The bundle model is the multi-table workbench layer:

- `Locus`: named collection of related, potentially ragged `Catalog` values
- `LocusTraverse`: fluent link-graph builder returned by `Locus::from(...)`
- `LocusPull`: root-grain feature builder returned by `Locus::pull(...)`
- `LocusQuery`: workspace SQL accessor returned by `Locus::query()`
- `Nexus`: named collection of related `DataFrame` tables
- `NexusTraverse`: fluent join-graph builder returned by `Nexus::from(...)`
- `NexusQuery`: workspace SQL accessor returned by `Nexus::query()`

Use it when the job is bigger than one table: several frames must be coordinated,
relationships should be declared once and reused, and join plans or audits should
belong to the workspace itself rather than be rebuilt ad hoc each time.

Choose Locus when ragged `Catalog` structure is the public boundary. Choose Nexus
when every member is already a rectangular `DataFrame`. Locus aligns only for an
operation that requires a frame workspace and converts data results back to
Catalog, so callers do not have to abandon the ragged model merely to declare
links, traverse them, pull root-grain features, or issue a query.

See the [Locus API Reference](locus_api_reference.md),
[LocusTraverse API Reference](locus_traverse_api_reference.md),
[LocusPull API Reference](locus_pull_api_reference.md), and
[LocusQuery API Reference](locus_query_api_reference.md) for the ragged workspace
surface.

## Nexus

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name", pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id", pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount", pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
}).add_relationship({
    .name = "customer_orders",
    .left_table = "customers",
    .right_table = "orders",
    .keys = {{"customer_id", "customer_id"}},
});

auto joined = biz.from("customers").join("orders").collect();
```

## What Nexus Adds Beyond DataFrame

- keyed table storage with `frame(...)`, `select(...)`, `drop(...)`, and `inject(...)`
- declared relationship metadata with `add_relationship(...)`
- root-grain feature assembly with `pull(...)`
- audit surfaces such as `audit()`, `validate_keys(...)`, `validate_relationship(...)`, and `detect_orphans(...)`
- graph navigation with `path(...)`, `plan(...)`, `materialize(...)`, and `from(...).via(...)`
- persistence with `save(...)`, `load(...)`, and `store(...)`

See the [Nexus API Reference](nexus_api_reference.md) and
[NexusTraverse API Reference](nexus_traverse_api_reference.md) and
[NexusQuery API Reference](nexus_query_api_reference.md) for the full public surface.
