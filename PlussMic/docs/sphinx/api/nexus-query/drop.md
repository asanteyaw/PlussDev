<!-- pluss-hand-authored -->

# NexusTraverse::drop

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signature

```cpp
NexusTraverse NexusTraverse::drop(std::initializer_list<std::string_view> features) const
NexusTraverse NexusTraverse::drop(pluss::msharp::RangeView::Of<std::string_view> features) const
template <detail::StringViewNameRange NameRange>
NexusTraverse NexusTraverse::drop(const NameRange& features) const
```

## What It Does

Removes the named columns from what [`.collect()`](collect.md) returns, keeping everything
else — the complement of [`.pick()`](pick.md). Applied after every join step, same as
`.pick()`. Calling `.drop()` again replaces the previous drop list rather than combining
with it.

## Parameters

**features** (a range of `std::string_view` — a brace literal, a named
container such as `std::vector<std::string_view>`/`std::vector<std::string>`,
or any other range whose elements convert to `std::string_view`)
    The feature names to remove from the final result. Same validation as
    [`.pick()`](pick.md): each name must be non-empty and unique within the request.
    Existence is checked when [`.collect()`](collect.md) runs.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
See [`Nexus::select`](../nexus/select.md)'s own note on this — the same
three-overload pattern (brace literal / named `RangeView`-convertible
container / any other qualifying range) applies here identically.
```

## Returns

`NexusTraverse`
    A new query with the drop list set (or replaced). The original query is unchanged.

## Examples

Example 1. Drop the redundant join key after joining.

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

auto result = linked.from("orders").join("customers").drop({"customer_id"}).collect();

for (const auto& f : result.features()) {
  std::cout << f << " ";
}
```

### Output

```text
order_id amount name
```

`customer_id` (the join key, now redundant once the frames are joined) is gone;
`order_id`, `amount`, and `name` all survive, in the joined frame's own original order.

Example 2. Continuing from Example 1: the same drop list via named
containers — a `std::vector<std::string_view>` (the `RangeView` overload)
and a `std::vector<std::string>` (the templated overload).

```cpp
std::vector<std::string_view> as_view = {"customer_id"};
std::vector<std::string> as_owned = {"customer_id"};

auto r1 = linked.from("orders").join("customers").drop(as_view).collect();
auto r2 = linked.from("orders").join("customers").drop(as_owned).collect();

for (const auto& f : r1.features()) std::cout << f << " ";
std::cout << "\n";
for (const auto& f : r2.features()) std::cout << f << " ";
```

### Output

```text
order_id amount name
order_id amount name
```

## Notes

- Applies to the final joined result, after [`.pick()`](pick.md) if both are set — see
  that page's Notes for the exact application order.
- A second `.drop()` call replaces the earlier list rather than combining with it.

## See Also

- [`NexusTraverse::pick`](pick.md): the complementary "keep only these" operation.
- [`NexusTraverse::collect`](collect.md): where `.pick()`/`.drop()` are actually applied.
- [`Nexus::drop`](../nexus/drop.md): the table-level (not column-level) analog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pick.html">
    <span class="label">Previous</span>
    <span class="title">pick</span>
  </a>
  <a class="next-link" href="name.html">
    <span class="label">Next</span>
    <span class="title">name</span>
  </a>
</div>
