<!-- pluss-hand-authored -->

# NexusQuery::collect

[Back to NexusQuery API Reference](../../nexus_query_api_reference)

## Signature

```cpp
NexusQuery::collect() const -> DataFrame
```

## What It Does

Executes the query's accumulated [`JoinPlan`](../nexus/plan.md) — via
[`Nexus::materialize()`](../nexus/materialize.md) — and applies, in order:

1. [`.pick()`](pick.md), if set — narrow to exactly the named columns.
2. [`.drop()`](drop.md), if set — remove the named columns from what's left.
3. [`.name()`](name.md), if set — retitle the result.

This is the only `NexusQuery` method that actually touches table data; every other method
on this class (`.join()`, `.via()`, `.pick()`, `.drop()`, `.name()`) only builds up the
plan and returns a new, still-unexecuted `NexusQuery`.

## Parameters

`None`

## Returns

`DataFrame`
    The materialized, joined (and, if requested, projected and retitled) result.

## Examples

Example 1. A query with no `.join()` calls collects to the anchor table itself.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});

auto biz = Nexus({{"customers", customers}});

auto result = biz.from("customers").collect();
std::cout << result.ncases() << " " << result.nfeatures();
```

### Output

```text
3 2
```

Example 2. A full pipeline: join, pick, and name, all applied by one `.collect()` call.

```cpp
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
});

auto with_orders = biz.inject("orders", orders);

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = with_orders.add_relationship(rel);

auto pipeline = linked.from("orders")
                    .join("customers")
                    .pick({"order_id", "name"})
                    .name("orders_with_customer_name");

auto result2 = pipeline.collect();
std::cout << result2.title() << " ";
for (const auto& f : result2.features()) {
  std::cout << f << " ";
}
```

### Output

```text
orders_with_customer_name order_id name
```

Nothing above `.collect()` touched any table data — `pipeline` itself is just a recipe
until this call runs it.

## Notes

- The only data-touching method on `NexusQuery` — everything else builds the plan lazily.
- Applies `.pick()` before `.drop()`, and `.name()` last — see the numbered list above.
- Calling `.collect()` twice on the same query re-executes the plan each time; nothing is
  cached on the `NexusQuery` object itself (though [`Nexus::materialize()`](../nexus/materialize.md)'s
  own internals may still benefit from the workspace-level caches
  [`path()`](../nexus/path.md)/[`plan()`](../nexus/plan.md) maintain).

## See Also

- [`Nexus::materialize`](../nexus/materialize.md): the underlying execution this method
  calls.
- [`NexusQuery::pick`](pick.md) / [`drop`](drop.md) / [`name`](name.md): the three
  post-processing steps applied here, in order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="name.html">
    <span class="label">Previous</span>
    <span class="title">name</span>
  </a>
</div>
