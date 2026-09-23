# Part XII-B: Bundle: Nexus, NexusTraverse, and NexusQuery

Bundle is PlussMic's multi-table workbench model. It sits beside the table
model rather than replacing it: each table is still a `DataFrame`, but the
workspace that coordinates them is a `Nexus`.

## When to use Bundle

Reach for Bundle when:

- your analysis spans several named tables;
- join rules should be declared once and reused;
- you need root-grain feature assembly, key audits, orphan detection, or relationship validation;
- a saved multi-hop join plan is part of the workflow.

## Nexus

`Nexus` stores a keyed set of `DataFrame` objects and relationship metadata.

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
```

From there you can:

- fetch tables with `frame(...)`
- inspect workspace state with `summary()`, `profile()`, and `schema()`
- assemble root-grain feature tables with `pull(...)`
- validate relationships and workspace health with `audit()`, `validate_keys(...)`, `validate_relationship(...)`, and `relationship_audit(...)`
- persist the workspace with `save(...)`, `load(...)`, and `store(...)`

The practical split is:

- `pull(...)` for grain-preserving feature assembly back onto a root table
- `from(...).join(...).collect()` when you actually want the expanded child rows
- `audit()` when you want one command-center view of table-key and relationship integrity

## NexusTraverse

`NexusTraverse` is the fluent graph-walk builder returned by `Nexus::from(...)`.

```cpp
auto joined = biz.from("customers")
                 .join("orders")
                 .pick({"customer_id", "name", "amount"})
                 .collect();
```

## NexusQuery

`NexusQuery` is the SQL workspace accessor returned by `Nexus::query()`.

```cpp
auto totals = biz.query().execute(
    "SELECT customers.customer_id, orders.amount "
    "FROM customers "
    "JOIN orders ON customers.customer_id = orders.customer_id");
```

The important distinction is now architectural:

- `Nexus` is the retained workspace object
- `NexusTraverse` is the lazy graph-walk builder returned by `from(...)`
- `NexusQuery` is the SQL-facing workspace query accessor returned by `query()`
- `DataFrame` is still the concrete result after either `.collect()` or SQL execution

For exact signatures and verified examples, use the
[Nexus API Reference](../nexus_api_reference.md) and
[NexusTraverse API Reference](../nexus_traverse_api_reference.md) and
[NexusQuery API Reference](../nexus_query_api_reference.md).
