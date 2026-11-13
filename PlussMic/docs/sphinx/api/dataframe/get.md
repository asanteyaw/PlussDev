<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 936a189b353a -->

# DataFrame::get

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
std::optional<Slate> DataFrame::get(std::string_view feature_name) const
```

## What It Does

Looks up a single feature by name and returns it wrapped in `std::optional<Slate>` —
`std::nullopt` if no feature with that name exists, instead of raising. This is the
"ask first" counterpart to [`features(name)`](features.md), which returns the column
directly but raises when the name doesn't exist. Reach for `get()` whenever the
feature name comes from somewhere you don't fully control (user input, a config file,
an upstream schema you can't guarantee) and a missing column should be handled rather
than treated as a programming error.

## Parameters

**feature_name** (`std::string_view`)

The feature name to look up.

## Returns

`std::optional<Slate>`
    The requested column, labeled with the source frame's case labels and its own
    feature name as its display name, if `feature_name` exists on the frame.
    `std::nullopt` otherwise.

## Examples

Example 1. Look up a feature that exists.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"order_id",   Slate(pluss::make_series({101, 102, 103, 104, 105, 106}))},
    {"units",      Slate(pluss::make_series({2, 5, 1, 3, 4, 2}))},
    {"unit_price", Slate(pluss::make_series({1200.0, 45.5, 1150.0, 320.0, 42.0, std::numeric_limits<double>::quiet_NaN()}))},
});

auto units = orders.get("units");
if (units.has_value()) {
    std::cout << *units;
}
```

### Output

```text
0  2
1  5
2  1
3  3
4  4
5  2
Name: units, ptype: Series
```

`units` is a real feature, so `get()` returns an engaged `std::optional<Slate>`
holding the same column [`features("units")`](features.md) would return.

Example 2. Look up a feature that doesn't exist.

```cpp
auto missing = orders.get("customer_email");
std::cout << std::boolalpha << missing.has_value();
```

### Output

```text
false
```

There is no `customer_email` feature on `orders`, so `get()` returns `std::nullopt`
rather than raising — the call site decides how to handle the absence (fall back to a
default, skip the column, report a schema mismatch, and so on).

## Notes

- `get()` never raises for an unknown feature name; that's the entire reason it exists
  alongside [`features(name)`](features.md), which does raise.
- The returned `Slate`, when present, carries the feature's name as its display name
  and the frame's case labels as its display labels — identical to what
  `features(name)` returns.
- `get()` only looks up a single feature by exact name; it has no fuzzy-matching or
  case-insensitivity built in.

## See Also

- [`DataFrame::features`](features.md): the raising counterpart, for when a missing
  feature should be a hard error.
- [`DataFrame::has`](has.md): checks whether a value exists within a feature's data,
  a different question from whether a feature exists at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="group.html">
    <span class="label">Next</span>
    <span class="title">group</span>
  </a>
</div>
