<!-- pluss-hand-authored -->

# Nexus::names

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::names() const -> const std::vector<std::string>&
```

## What It Does

An exact alias for [`keys()`](keys.md) — `names()` calls `keys()` internally and
returns the identical reference. Both names exist because a `Nexus` workspace reads
naturally either as a dictionary (`keys()`, paralleling `has(key)`/`get(key)`) or as
a named collection of tables (`names()`, paralleling how
[`DataFrame::features()`](../dataframe/features.md) names columns). Pick whichever
reads better at the call site — there is no behavioral difference.

## Parameters

`None`

## Returns

`const std::vector<std::string>&`
    The table names, in workspace order — identical to [`keys()`](keys.md).

## Examples

Example 1. `names()` and `keys()` always agree.

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

std::cout << (biz.names() == biz.keys());
```

### Output

```text
1
```

`std::vector<std::string>::operator==` reports `true` (printed as `1` by `<<`,
since it's a plain `bool`) — `names()` returns the very same underlying data as
`keys()`, not merely an equal copy.

## Notes

- Purely a naming alternative — see [`keys()`](keys.md) for the full documentation.

## See Also

- [`Nexus::keys`](keys.md): the canonical name for this same method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="meta.html">
    <span class="label">Previous</span>
    <span class="title">meta</span>
  </a>
  <a class="next-link" href="neighbors.html">
    <span class="label">Next</span>
    <span class="title">neighbors</span>
  </a>
</div>
