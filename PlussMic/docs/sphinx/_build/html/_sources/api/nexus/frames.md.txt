<!-- pluss-hand-authored -->

# Nexus::frames

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::frames() const -> const std::vector<DataFrame>&
```

## What It Does

Returns every table in the workspace as a plain vector of `DataFrame`, in the same
order as [`keys()`](keys.md)/[`names()`](names.md) — `frames()[i]` is always the
table named `keys()[i]`. Useful for iterating over every table without knowing (or
caring about) their names, or for zipping the two vectors together.

## Parameters

`None`

## Returns

`const std::vector<DataFrame>&`
    The tables, in workspace order.

## Examples

Example 1. Iterate over both vectors together to print each table's shape.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

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

const auto& keys = biz.keys();
const auto& frames = biz.frames();
for (std::size_t i = 0; i < frames.size(); ++i) {
  std::cout << keys[i] << ": " << frames[i].ncases() << " cases, "
            << frames[i].nfeatures() << " features\n";
}
```

### Output

```text
customers: 3 cases, 2 features
orders: 3 cases, 3 features
```

## Notes

- Returned by reference — reading it doesn't copy any table.
- Order always matches [`keys()`](keys.md)/[`names()`](names.md) positionally.
- To get one table by name (rather than iterating all of them), prefer
  [`frame(key)`](frame.md) or [`get(key)`](get.md).

## See Also

- [`Nexus::keys`](keys.md): the matching table names, in the same order.
- [`Nexus::frame`](frame.md): fetch exactly one table, by name or position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="frame.html">
    <span class="label">Previous</span>
    <span class="title">frame</span>
  </a>
  <a class="next-link" href="from.html">
    <span class="label">Next</span>
    <span class="title">from</span>
  </a>
</div>
