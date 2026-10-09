<!-- pluss-hand-authored -->

# Nexus::materialize

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::materialize(const JoinPlan& plan) const -> DataFrame
```

## What It Does

Executes a [`JoinPlan`](plan.md), chaining a [`join()`](join.md)-equivalent call for each
step in order, and returns the final joined `DataFrame`. This is the method that actually
touches table data among the `path()`/`plan()`/`materialize()` trio —
[`path()`](path.md) and [`plan()`](plan.md) only compute *how* to join; `materialize()` is
where it happens.

A `JoinPlan` with zero steps (for example, from `plan(path("x", "x"))`, since `from == to`
produces a zero-step path) just returns the root table itself, retitled.

## Parameters

**plan** (`const JoinPlan&`)
    The recipe to execute — typically built by [`plan()`](plan.md), but can be constructed
    by hand. `plan.root_table` must be non-empty and must already exist in the workspace;
    each step's `right_table` must also exist.

## Returns

`DataFrame`
    The result of chaining every step's join in order, titled `plan.result_name` (falling
    back to `plan.root_table` for a zero-step plan, or the stable
    `root_table + "_" + <last right table>` name otherwise).

## Examples

Example 1. Materializing a two-hop plan.

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

auto plan_value = linked.plan("customers", "payments", JoinHow::Left);
auto materialized = linked.materialize(plan_value);

std::cout << materialized.ncases() << " " << materialized.nfeatures() << "\n";
for (const auto& f : materialized.features()) {
  std::cout << f << " ";
}
```

### Output

```text
4 5
customer_id name order_id amount payment_id
```

Every `customers` row survives the left join to `orders` (customer `3` gets a null-filled
row, since it has no orders); every one of those rows then left-joins to `payments`, which
only has entries for orders `101` and `102`.

## Notes

- The only method in the `path()`/`plan()`/`materialize()` trio that actually joins table
  data — the other two only compute the recipe.
- A zero-step plan (root equals destination) just returns the root table, retitled.
- Each step resolves the same way [`join()`](join.md) does — see that page for the full
  `JoinSpec` resolution rules, including how `relationship_name` fills in `keys`.

## See Also

- [`Nexus::plan`](plan.md) / [`path`](path.md): build the recipe this method executes.
- [`Nexus::join`](join.md): the single-hop primitive each step is equivalent to.
- [`Nexus::run`](run.md): materialize a previously-[`store`](store.md)d plan by name.
- [`NexusTraverse::collect`](../../nexus_traverse_api_reference): the fluent builder's own way to
  reach this same execution step.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_frames.html">
    <span class="label">Previous</span>
    <span class="title">map_frames</span>
  </a>
  <a class="next-link" href="meta.html">
    <span class="label">Next</span>
    <span class="title">meta</span>
  </a>
</div>
