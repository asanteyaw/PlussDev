<!-- pluss-hand-authored -->

# Nexus::plan

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::plan(const JoinPath& path, JoinHow how = JoinHow::Left) const -> JoinPlan

Nexus::plan(std::string_view from, std::string_view to, JoinHow how = JoinHow::Left) const -> JoinPlan
```

## What It Does

Turns a [`JoinPath`](path.md) — or, in the convenience overload, a `(from, to)` pair that
gets resolved to a path internally — into an executable `JoinPlan`, applying the same join
kind (`how`) to every hop. `plan()` alone doesn't touch any table data; it only builds the
recipe. [`materialize()`](materialize.md) is the method that actually executes a `JoinPlan`
and returns the joined `DataFrame`.

### `JoinPlanStep` fields

| Field | Type | Default |
|---|---|---|
| `left_table` | `std::string` | empty |
| `right_table` | `std::string` | empty |
| `spec` | `JoinSpec` | default constructed |

### `JoinPlan` fields

| Field | Type | Default |
|---|---|---|
| `root_table` | `std::string` | empty |
| `result_name` | `std::string` | empty |
| `steps` | `std::vector<JoinPlanStep>` | empty |

Each `JoinPlanStep` is a fully-formed `JoinSpec` (its `relationship_name` set to the
declaration the corresponding `JoinPathStep` used, `right_suffix` defaulted to
`"_" + right_table`), ready to feed straight into [`Nexus::join()`](join.md)-style
resolution. `result_name` defaults to `root_table + "_" + <last hop's right table>` — the
same naming convention [`join()`](join.md) uses for a single hop.

## Parameters

**path** (`const JoinPath&`, first overload)
    A route already computed by [`path()`](path.md) (or built by hand).

**from** / **to** (`std::string_view`, second overload)
    Table names to connect — internally calls `path(from, to)` first, so the same
    "no route found" throw applies. Results are cached per `(from, to, how)`.

**how** (`JoinHow`, default `Left`)
    The join kind applied uniformly to every hop in the plan.

## Returns

`JoinPlan`
    An executable multi-hop join recipe, ready for [`materialize()`](materialize.md).

## Examples

Example 1. Building a plan from an already-computed path.

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

auto p = linked.path("customers", "payments");
auto plan_value = linked.plan(p, JoinHow::Left);

std::cout << plan_value.root_table << " -> " << plan_value.result_name
          << " steps=" << plan_value.steps.size();
```

### Output

```text
customers -> customers_payments steps=2
```

Example 2. The convenience overload does the `path()` call for you.

```cpp
auto plan_value2 = linked.plan("customers", "payments", JoinHow::Left);
std::cout << plan_value2.root_table << " -> " << plan_value2.result_name;
```

### Output

```text
customers -> customers_payments
```

## Notes

- `result_name` defaults to `root_table + "_" + <last hop's right table>`.
- `how` applies uniformly across every hop — there's no per-hop override on this method;
  build a `JoinPlan` by hand (or edit the returned one's `steps`) if different hops need
  different join kinds.
- The two-argument-plus-`how` overload is cached per `(from, to, how)`, same as
  [`path()`](path.md)'s own `(from, to)` caching.

## See Also

- [`Nexus::path`](path.md): compute the route this method turns into a plan.
- [`Nexus::materialize`](materialize.md): actually execute the plan and get back a
  `DataFrame`.
- [`Nexus::store`](store.md): save a `JoinPlan` under a name for reuse via
  [`run()`](run.md).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="path.html">
    <span class="label">Previous</span>
    <span class="title">path</span>
  </a>
  <a class="next-link" href="profile.html">
    <span class="label">Next</span>
    <span class="title">profile</span>
  </a>
</div>
