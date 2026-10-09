<!-- pluss-hand-authored -->

# NexusTraverse::name

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signature

```cpp
NexusTraverse::name(std::string_view value) const -> NexusTraverse
```

## What It Does

Sets the title [`.collect()`](collect.md)'s returned `DataFrame` will carry, overriding
the automatic `root_table + "_" + <last table>`-style name the underlying
[`JoinPlan`](../nexus/plan.md) would otherwise use.

## Parameters

**value** (`std::string_view`)
    The title to apply. Must be non-empty — an empty string throws
    `"NexusTraverse::name: title cannot be empty"`.

## Returns

`NexusTraverse`
    A new query with the title set (or replaced). The original query is unchanged.

## Examples

Example 1. Naming the result of a join.

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

auto result = linked.from("orders").join("customers").name("orders_with_customers").collect();
std::cout << result.title();
```

### Output

```text
orders_with_customers
```

Example 2. An empty title throws.

```cpp
try {
  auto bad = linked.from("orders").name("");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
NexusTraverse::name: title cannot be empty
```

Note that the throw happens immediately when `.name("")` is called — before `.collect()`
even runs.

## Notes

- Validated (non-empty) immediately when `.name()` is called, not deferred to
  [`.collect()`](collect.md).
- A second `.name()` call replaces the earlier title rather than appending to it.
- Without a call to `.name()`, [`.collect()`](collect.md) falls back to the underlying
  `JoinPlan`'s own `result_name` — see [`Nexus::plan()`](../nexus/plan.md) for that default
  naming rule.

## See Also

- [`NexusTraverse::collect`](collect.md): where the title is actually applied, via
  [`DataFrame::title()`](../dataframe/title.md).
- [`Nexus::plan`](../nexus/plan.md): the default naming rule this method overrides.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="collect.html">
    <span class="label">Next</span>
    <span class="title">collect</span>
  </a>
</div>
