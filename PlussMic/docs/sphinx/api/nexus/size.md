<!-- pluss-hand-authored -->

# Nexus::size

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::size() const -> std::int64_t
```

## What It Does

An exact synonym for [`ntables()`](ntables.md) — `size()` calls `ntables()`
internally and returns precisely the same value. It exists purely for readers coming
from STL-container-style naming conventions (`.size()`), alongside the more
domain-specific `ntables()` name used everywhere else in this reference (`ncases()`,
`nfeatures()`, and so on, on [`DataFrame`](../dataframe/ncases.md)).

## Parameters

`None`

## Returns

`std::int64_t`
    The number of tables currently in the workspace — identical to
    [`ntables()`](ntables.md).

## Examples

Example 1. `size()` and `ntables()` always agree.

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

std::cout << biz.size() << " " << biz.ntables();
```

### Output

```text
2 2
```

## Notes

- Prefer [`ntables()`](ntables.md) in new code for clarity — `size()` reads more
  ambiguously next to `DataFrame::ncases()`/`nfeatures()`, which count rows/columns
  within one table, not tables within a workspace.

## See Also

- [`Nexus::ntables`](ntables.md): the canonical name for this same value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="store.html">
    <span class="label">Next</span>
    <span class="title">store</span>
  </a>
</div>
