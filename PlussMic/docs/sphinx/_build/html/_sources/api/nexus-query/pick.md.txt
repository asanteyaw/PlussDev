<!-- pluss-hand-authored -->

# NexusTraverse::pick

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signature

```cpp
NexusTraverse::pick(std::initializer_list<std::string_view> features) const -> NexusTraverse
```

## What It Does

Narrows the columns [`.collect()`](collect.md) returns to exactly the ones named, in the
order given — a projection applied *after* every join step, not a per-table column
selection before joining. Calling `.pick()` again on a query that already has a pick list
replaces it entirely; it doesn't intersect or extend the previous list.

## Parameters

**features** (`std::initializer_list<std::string_view>`)
    The feature names to keep in the final result, in the order they should appear.
    Validated the same way as other name lists on this class (`Nexus::select()`'s keys,
    for example): each name must be non-empty and unique within the request. Feature
    existence itself is checked only when [`.collect()`](collect.md) actually runs and
    tries to select them from the materialized frame.

## Returns

`NexusTraverse`
    A new query with the pick list set (or replaced). The original query is unchanged.

## Examples

Example 1. Narrow the joined result to two columns.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
    {"amount",      Thread(pluss::make_series({50.0, 75.0, 20.0}))},
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

auto result = linked.from("orders").join("customers").pick({"order_id", "name"}).collect();

for (const auto& f : result.features()) {
  std::cout << f << " ";
}
```

### Output

```text
order_id name
```

## Notes

- Applies to the final joined result, not to any individual table before joining.
- A second `.pick()` call replaces the earlier list rather than combining with it.
- `.pick()` and [`.drop()`](drop.md) are independent — using both on the same query is
  legal, but redundant, since `.collect()` applies `.pick()` first (narrowing to the
  picked set) and `.drop()` second (dropping from what's left).

## See Also

- [`NexusTraverse::drop`](drop.md): the complementary "remove these, keep everything else"
  operation.
- [`NexusTraverse::collect`](collect.md): where `.pick()`/`.drop()` are actually applied.
- [`Nexus::select`](../nexus/select.md): the table-level (not column-level) analog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="via.html">
    <span class="label">Previous</span>
    <span class="title">via</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
