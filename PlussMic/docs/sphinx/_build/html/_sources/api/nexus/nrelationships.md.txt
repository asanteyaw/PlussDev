<!-- pluss-hand-authored -->

# Nexus::nrelationships

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::nrelationships() const -> std::int64_t
```

## What It Does

Returns how many relationships are declared in the workspace — `relationships().size()`,
returned as `std::int64_t` to match this class's other count-style methods
([`ntables()`](ntables.md), [`size()`](size.md)) rather than `std::vector`'s own
`size_type`.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of declared relationships. `0` for a workspace with none.

## Examples

Example 1. Counting relationships before and after declaring one.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
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

std::cout << biz.nrelationships() << " " << linked.nrelationships();
```

### Output

```text
0 1
```

## Notes

- Equivalent to `relationships().size()`, returned as `std::int64_t`.
- `biz` itself never changes — [`add_relationship()`](add_relationship.md) is non-mutating,
  so its relationship count stays `0` while the returned `linked` workspace's count becomes
  `1`.

## See Also

- [`Nexus::relationships`](relationships.md): the declarations themselves.
- [`Nexus::add_relationship`](add_relationship.md): declare a new relationship.
- [`Nexus::ntables`](ntables.md): the analogous count for tables instead of relationships.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="neighbors.html">
    <span class="label">Previous</span>
    <span class="title">neighbors</span>
  </a>
  <a class="next-link" href="ntables.html">
    <span class="label">Next</span>
    <span class="title">ntables</span>
  </a>
</div>
