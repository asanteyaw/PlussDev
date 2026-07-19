<!-- pluss-hand-authored -->

# DataFrame::ptypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ptypes() const -> DTypes
```

## What It Does

Returns the coarse payload family of every feature in the frame — `Series`, `Note`,
`Timeline`, or `TimeIndex` — without the concrete scalar type underneath. Where
[`dtypes()`](dtypes.md) tells you `Series<float64>` versus `Series<int64>`,
`ptypes()` only tells you it's a `Series` at all. Reach for `ptypes()` when you're
routing logic by payload family (numeric work needs `Series`, text work needs `Note`,
and so on) and don't need the extra precision `dtypes()` provides.

Like [`dtypes()`](dtypes.md), `ptypes()` returns a `DataFrame::DTypes` object rather
than a raw map — it supports `size()`, `empty()`, `contains(name)`,
`at(name)`/`operator[](name)`, and range-based iteration over `(name, ptype)` pairs,
and has its own `operator<<` for a quick printout.

## Parameters

`None`

## Returns

`DTypes`
    A feature-to-payload-family summary. Iterate it directly, or look up one
    feature's payload family with `ptypes()["feature_name"]` /
    `ptypes().at("feature_name")`.

## Examples

Example 1. Inspect payload families without the concrete scalar type.

```cpp
using namespace pluss::table;

auto users = DataFrame({
    {"user_id",     pluss::make_series({101.0, 102.0, 103.0})},
    {"signup_note", pluss::make_note({"referral", "ad", "referral"})},
});

auto out = users.ptypes();
std::cout << out;
```

### Output

```text
features  dtype
--------  --------
user_id  Series
signup_note  Note
```

`ptypes()` reports `Series` for `user_id` regardless of whether it's stored as
`float64` or `int64` underneath — for that level of detail, use
[`dtypes()`](dtypes.md) instead. As with `dtypes()`, the row alignment above is not a
typo: `DTypes::operator<<` prints a fixed header and separator, then each `name  ptype`
pair with a plain two-space gap and no column-width padding.

Example 2. Look up one feature's payload family directly.

```cpp
auto out = users.ptypes();
if (out.contains("signup_note")) {
  std::cout << out["signup_note"] << "\n";  // "Note"
}
```

## Notes

- `ptypes()` reports the *coarse* payload family only (`Series`, `Note`, `Timeline`,
  `TimeIndex`). Use [`dtypes()`](dtypes.md) when you need the concrete scalar type
  (`Series<float64>` vs `Series<int64>`) as well.
- Iteration order matches the frame's current feature order, the same order
  [`features()`](features.md) returns.

## See Also

- [`DataFrame::dtypes`](dtypes.md): the concrete-dtype counterpart.
- [`DataFrame::feataxis`](feataxis.md): select or route features by payload family.
- [`DataFrame::cast`](cast.md), [`DataFrame::astype`](astype.md): change a feature's
  dtype (and, for `Timeline`/`TimeIndex`, its payload family) after inspecting it here.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="put.html">
    <span class="label">Next</span>
    <span class="title">put</span>
  </a>
</div>
