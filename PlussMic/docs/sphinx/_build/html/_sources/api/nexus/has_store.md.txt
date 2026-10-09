<!-- pluss-hand-authored -->

# Nexus::has_store

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::has_store(std::string_view name) const -> bool
```

## What It Does

Reports whether a named, saved [`JoinPlan`](plan.md) — a "view" — exists in the workspace.
Saved views are created by [`store()`](store.md) and executed later by name via
[`run()`](run.md), without having to rebuild the [`JoinPlan`](plan.md) each time.

## Parameters

**name** (`std::string_view`)
    The view name to check.

## Returns

`bool`
    `true` if a view with that exact name has been saved, `false` otherwise.

## Examples

Example 1. Before and after saving a view.

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

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = biz.add_relationship(rel);

auto plan_value = linked.plan("orders", "customers", JoinHow::Left);
auto stored = linked.store("orders_with_customers", plan_value);

std::cout << std::boolalpha << linked.has_store("orders_with_customers") << " "
          << stored.has_store("orders_with_customers");
```

### Output

```text
false true
```

`linked` is left without the saved view — [`store()`](store.md) is non-mutating.

## Notes

- Never throws, regardless of the name given.

## See Also

- [`Nexus::store`](store.md): save a `JoinPlan` under a name.
- [`Nexus::run`](run.md): execute a saved view by name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_relationship.html">
    <span class="label">Previous</span>
    <span class="title">has_relationship</span>
  </a>
  <a class="next-link" href="has_table_meta.html">
    <span class="label">Next</span>
    <span class="title">has_table_meta</span>
  </a>
</div>
