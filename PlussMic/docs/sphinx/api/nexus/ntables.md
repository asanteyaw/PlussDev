<!-- pluss-hand-authored -->

# Nexus::ntables

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
std::int64_t Nexus::ntables() const
```

## What It Does

Returns the number of tables currently held in the workspace. This is the canonical
count — [`size()`](size.md) is an exact synonym that delegates to this method.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of tables in the workspace.

## Examples

Example 1. A two-table workspace reports `2`.

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

std::cout << biz.ntables();
```

### Output

```text
2
```

Example 2. Adding a table with `inject_()` increases the count.

```cpp
auto products = DataFrame({
    {"product_id", pluss::make_series({1.0, 2.0})},
    {"label",      pluss::make_note({"Widget", "Gadget"})},
});

biz.inject_("products", products);
std::cout << biz.ntables();
```

### Output

```text
3
```

## Notes

- `ntables()` needs a defined handle — calling it on an undefined (e.g. moved-from)
  `Nexus` throws `"Nexus: undefined handle"`, same as most other methods. See
  [`defined()`](defined.md).
- [`size()`](size.md) is a pure alias, provided for STL-style familiarity.

## See Also

- [`Nexus::size`](size.md): an exact synonym for this value.
- [`Nexus::empty`](empty.md): a boolean shortcut for `ntables() == 0` (or undefined).
- [`Nexus::keys`](keys.md): the table names themselves, in the same count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrelationships.html">
    <span class="label">Previous</span>
    <span class="title">nrelationships</span>
  </a>
  <a class="next-link" href="orphans.html">
    <span class="label">Next</span>
    <span class="title">orphans</span>
  </a>
</div>
