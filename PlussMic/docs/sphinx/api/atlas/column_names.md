<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5451b8b41445 -->

# Atlas::column_names

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::column_names() const -> const std::vector<std::string>&
```

## What It Does

Returns the ordered feature names shared by every frame in the stack — equivalent to
calling [`features()`](../dataframe/features.md) (the no-argument overload) on any one
frame in the atlas, since the strict-cube guarantee means they all agree. This is a
borrowed reference into the atlas's internal state, not a copy; it stays valid as long as
the `Atlas` that produced it is alive.

## Parameters

`None`

## Returns

`const std::vector<std::string>&`
    The feature names, in their shared left-to-right order. Empty for an atlas built
    from an empty frame vector.

## Examples

Example 1. List the feature names shared across the stack.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"sales",  Thread(pluss::make_series({100.0, 120.0}))},
    {"orders", Thread(pluss::make_series({12.0, 15.0}))},
});
auto q2 = DataFrame({
    {"sales",  Thread(pluss::make_series({110.0, 90.0}))},
    {"orders", Thread(pluss::make_series({14.0, 9.0}))},
});
auto atlas = Atlas({q1, q2});

for (const auto& name : atlas.column_names()) {
  std::cout << name << " ";
}
```

### Output

```text
sales orders
```

Example 2. An atlas built from an empty frame vector has no column names to report.

```cpp
using namespace pluss::table;

auto atlas = Atlas(std::vector<DataFrame>{});
std::cout << atlas.column_names().size();
```

### Output

```text
0
```

## Notes

- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.
- Returns a reference, not a copy — do not hold onto it past the lifetime of the `Atlas`
  it came from.
- Because of the strict-cube guarantee, this is the same name list every individual
  frame in the stack would report from its own `features()`.

## See Also

- [`Atlas::nfeatures`](nfeatures.md): the count matching this list's length.
- [`Atlas::features`](features.md): project the stack down to a named subset, changing
  this list.
- [`DataFrame::features`](../dataframe/features.md): the equivalent no-argument call on
  a single frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfeatures.html">
    <span class="label">Previous</span>
    <span class="title">nfeatures</span>
  </a>
  <a class="next-link" href="axis0_labels.html">
    <span class="label">Next</span>
    <span class="title">axis0_labels</span>
  </a>
</div>
