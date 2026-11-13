<!-- pluss-hand-authored -->

# NexusTraverse::via

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signature

```cpp
NexusTraverse NexusTraverse::via(std::string_view to, JoinHow how = JoinHow::Left) const
```

## What It Does

Extends the query by one or more hops in a single call, reaching `to` via the shortest
relationship-graph route from the current anchor — internally calling
[`Nexus::plan(anchor, to, how)`](../nexus/plan.md) (which itself calls
[`path()`](../nexus/path.md)) and appending every step of that plan to the query at once.
Where [`.join()`](join.md) adds exactly one explicit hop per call, `.via()` is the
"just get me there" shorthand when you don't want to spell out every intermediate table by
hand — useful when the route passes through a table you don't otherwise care about.

`how` applies uniformly to every hop `.via()` adds, same as [`Nexus::plan()`](../nexus/plan.md)'s
own `how` parameter — and defaults to `JoinHow::Left`, unlike [`.join()`](join.md)'s
`Inner` default.

## Parameters

**to** (`std::string_view`)
    The table to reach. Must already exist in the workspace, and a relationship path from
    the current anchor must exist — see [`path()`](../nexus/path.md)'s "no route" throw.

**how** (`JoinHow`, default `Left`)
    The join kind applied to every hop of the computed route.

## Returns

`NexusTraverse`
    A new query with every step of the route to `to` appended, anchor moved to `to`.

## Examples

Example 1. Reaching a two-hop-away table in one call.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 2.0}))},
    {"amount",      Slate(pluss::make_series({50.0, 75.0, 20.0}))},
});
auto payments = DataFrame({
    {"payment_id", Slate(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Slate(pluss::make_series({101.0, 102.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
    {"payments", payments},
});

Relationship orders_to_customers;
orders_to_customers.name = "orders_to_customers";
orders_to_customers.left_table = "orders";
orders_to_customers.right_table = "customers";
orders_to_customers.keys = {{"customer_id", "customer_id"}};

Relationship payments_to_orders;
payments_to_orders.name = "payments_to_orders";
payments_to_orders.left_table = "payments";
payments_to_orders.right_table = "orders";
payments_to_orders.keys = {{"order_id", "order_id"}};

auto linked = biz.add_relationships({orders_to_customers, payments_to_orders});

auto result = linked.from("customers").via("payments").collect();
std::cout << result.ncases();
```

### Output

```text
4
```

Reaching `payments` from `customers` requires two hops (through `orders`), which `.via()`
finds and joins in a single call. The default `Left` join keeps every `customers` row —
including customer `3`, who has no orders and therefore no payments — unlike the
`Inner`-by-default chained `.join()` calls shown on [that page](join.md)'s Example 2,
which drop unmatched rows at every hop.

## Notes

- `how` applies to every hop of the discovered route, same as
  [`Nexus::plan()`](../nexus/plan.md).
- Defaults to `JoinHow::Left`, unlike [`.join()`](join.md)'s `Inner` default.
- Throws the same "no relationship path" error as [`path()`](../nexus/path.md) if `to`
  isn't reachable from the current anchor at all.
- Non-mutating and chainable, like every other `NexusTraverse` method.

## See Also

- [`NexusTraverse::join`](join.md): one explicit hop at a time, defaulting to `Inner`.
- [`Nexus::path`](../nexus/path.md) / [`plan`](../nexus/plan.md): the lower-level
  primitives this method calls internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html">
    <span class="label">Previous</span>
    <span class="title">join</span>
  </a>
  <a class="next-link" href="pick.html">
    <span class="label">Next</span>
    <span class="title">pick</span>
  </a>
</div>
