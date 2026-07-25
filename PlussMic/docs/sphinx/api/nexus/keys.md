<!-- pluss-hand-authored -->

# Nexus::keys

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::keys() const -> const std::vector<std::string>&
```

## What It Does

Returns the table names in the workspace, in the same order they were constructed
(or last reordered by [`select()`](select.md)/[`rename()`](rename.md)/
[`inject()`](inject.md)/[`drop()`](drop.md)). This order is not alphabetical — it's
positional, and it's exactly the order [`frames()`](frames.md) uses too, so
`keys()[i]` and `frames()[i]` always describe the same table.

## Parameters

`None`

## Returns

`const std::vector<std::string>&`
    The table names, in workspace order.

## Examples

Example 1. Names come back in construction order.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", pluss::make_series({1.0, 2.0, 3.0})},
    {"name",        pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto orders = DataFrame({
    {"order_id",    pluss::make_series({100.0, 101.0, 102.0})},
    {"customer_id", pluss::make_series({1.0, 2.0, 2.0})},
    {"amount",      pluss::make_series({50.0, 75.0, 20.0})},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

for (const auto& key : biz.keys()) {
  std::cout << key << " ";
}
```

### Output

```text
customers orders
```

## Notes

- Returned by reference — no copy is made just to read the names.
- [`names()`](names.md) is an exact alias for this same method.
- The order matches [`frames()`](frames.md) positionally; use
  [`ntables()`](ntables.md) for the count without materializing the vector at all
  (though in practice `keys().size()` is just as cheap).

## See Also

- [`Nexus::names`](names.md): an exact alias.
- [`Nexus::frames`](frames.md): the tables themselves, in the same order.
- [`Nexus::has`](has.md): check for one specific key without scanning this vector
  yourself.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="inject.html">
    <span class="label">Previous</span>
    <span class="title">inject</span>
  </a>
  <a class="next-link" href="map_frames.html">
    <span class="label">Next</span>
    <span class="title">map_frames</span>
  </a>
</div>
