<!-- pluss-hand-authored -->

# NexusTraverse::pick

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signature

```cpp
NexusTraverse::pick(std::initializer_list<std::string_view> features) const -> NexusTraverse
NexusTraverse::pick(pluss::msharp::RangeView::Of<std::string_view> features) const -> NexusTraverse
template <detail::StringViewNameRange NameRange>
NexusTraverse::pick(const NameRange& features) const -> NexusTraverse
```

## What It Does

Narrows the columns [`.collect()`](collect.md) returns to exactly the ones named, in the
order given — a projection applied *after* every join step, not a per-table column
selection before joining. Calling `.pick()` again on a query that already has a pick list
replaces it entirely; it doesn't intersect or extend the previous list.

## Parameters

**features** (a range of `std::string_view` — a brace literal, a named
container such as `std::vector<std::string_view>`/`std::vector<std::string>`,
or any other range whose elements convert to `std::string_view`)
    The feature names to keep in the final result, in iteration order.
    Validated the same way as other name lists on this class (`Nexus::select()`'s keys,
    for example): each name must be non-empty and unique within the request. Feature
    existence itself is checked only when [`.collect()`](collect.md) actually runs and
    tries to select them from the materialized frame.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
See [`Nexus::select`](../nexus/select.md)'s own note on this — the same
three-overload pattern (brace literal / named `RangeView`-convertible
container / any other qualifying range) applies here identically.
```

## Returns

`NexusTraverse`
    A new query with the pick list set (or replaced). The original query is unchanged.

## Examples

Example 1. Narrow the joined result to two columns.

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

Example 2. Continuing from Example 1: the same pick list via named
containers — a `std::vector<std::string_view>` (the `RangeView` overload)
and a `std::vector<std::string>` (the templated overload).

```cpp
std::vector<std::string_view> as_view = {"order_id", "name"};
std::vector<std::string> as_owned = {"order_id", "name"};

auto r1 = linked.from("orders").join("customers").pick(as_view).collect();
auto r2 = linked.from("orders").join("customers").pick(as_owned).collect();

for (const auto& f : r1.features()) std::cout << f << " ";
std::cout << "\n";
for (const auto& f : r2.features()) std::cout << f << " ";
```

### Output

```text
order_id name
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
