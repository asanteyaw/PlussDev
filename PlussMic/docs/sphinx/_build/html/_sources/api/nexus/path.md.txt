<!-- pluss-hand-authored -->

# Nexus::path

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::path(std::string_view from, std::string_view to) const -> JoinPath
```

## What It Does

Finds a route between two tables through the declared relationship graph, using a
breadth-first search — the shortest route by number of hops, following relationship edges
in either direction (a relationship declared `left_table = "orders"`,
`right_table = "customers"` can be traversed from `orders` to `customers` *or* from
`customers` to `orders`; see [`neighbors()`](neighbors.md) for how each direction reports
itself). The result is a `JoinPath`, not a `DataFrame` — it describes *how* to get from
`from` to `to`, and feeds directly into [`plan()`](plan.md) to become an executable
[`JoinPlan`](plan.md).

```cpp
struct JoinPathStep {
  std::string relationship_name{};
  std::string from_table{};
  std::string to_table{};
  bool reversed{false};
};

struct JoinPath {
  std::string from{};
  std::string to{};
  std::vector<JoinPathStep> steps{};
};
```

Each `JoinPathStep` names the relationship being traversed and which direction it's being
traversed in (`reversed = true` when the step goes from that relationship's declared
`right_table` to its `left_table`).

## Parameters

**from** / **to** (`std::string_view`)
    The two tables to connect. Both must already exist in the workspace.

## Returns

`JoinPath`
    A route from `from` to `to`. If `from == to`, the returned path has zero steps. Results
    are cached internally per `(from, to)` pair.

## Examples

Example 1. A direct, one-hop path.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
});
auto payments = DataFrame({
    {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
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

auto p = linked.path("customers", "payments");
std::cout << p.from << " -> " << p.to << " steps=" << p.steps.size() << "\n";
for (const auto& step : p.steps) {
  std::cout << "  " << step.from_table << " -" << step.relationship_name << "-> "
            << step.to_table << " reversed=" << std::boolalpha << step.reversed << "\n";
}
```

### Output

```text
customers -> payments steps=2
  customers -orders_to_customers-> orders reversed=true
  orders -payments_to_orders-> payments reversed=true
```

No relationship directly connects `customers` and `payments`, so the search finds the
two-hop route through `orders`. Both steps are `reversed` because both relationships were
declared with `customers`/`orders` playing the *right*-hand role
(`orders_to_customers.right_table = "customers"`, `payments_to_orders.right_table =
"orders"`), and this path walks them right-to-left.

Example 2. No route at all throws.

```cpp
auto standalone = DataFrame({{"note", Thread(pluss::make_note({"unrelated"}))}});
auto with_standalone = linked.inject("standalone", standalone);

try {
  auto bad = with_standalone.path("customers", "standalone");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::path: no relationship path from 'customers' to 'standalone'
```

## Notes

- Finds the shortest route (fewest hops) via breadth-first search — not necessarily the
  *only* route, if the relationship graph has cycles or multiple connections between the
  same tables.
- `from == to` returns a zero-step path rather than throwing.
- Results are cached per `(from, to)` pair.
- Describes a route only — no actual joining happens until [`plan()`](plan.md) turns it
  into a `JoinPlan` and [`materialize()`](materialize.md) executes it.

## See Also

- [`Nexus::plan`](plan.md): turn this route into an executable `JoinPlan`.
- [`Nexus::neighbors`](neighbors.md): the one-hop adjacency this search traverses.
- [`Nexus::from`](from.md): the fluent [`NexusQuery`](../../nexus_query_api_reference)
  builder, which can use this same graph via `.via()` without you calling `path()`
  yourself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="orphans.html">
    <span class="label">Previous</span>
    <span class="title">orphans</span>
  </a>
  <a class="next-link" href="plan.html">
    <span class="label">Next</span>
    <span class="title">plan</span>
  </a>
</div>
