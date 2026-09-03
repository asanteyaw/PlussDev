<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 32b1b06ab8af -->

# DataFrame::dtypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::dtypes() const -> DTypes
```

## What It Does

Returns the concrete dtype of every feature in the frame — not just the coarse payload
family (`Series`, `Note`, `Timeline`, `TimeIndex`) but the specific scalar type
underneath, formatted as `"<Family><ConcreteType>"`, for example `Series<float64>`,
`Series<int64>`, or `Note<utf8>`. This is the schema-inspection method to reach for when
you need to know both *what kind* of column you have and *what precision or storage
type* it actually holds — before deciding whether a cast, a join, or a numeric operation
is safe.

`dtypes()` returns a `DataFrame::DTypes` object rather than a raw map or vector — an
ordered `(feature name, dtype string)` view. It supports `size()`, `empty()`,
`contains(name)`, `at(name)` / `operator[](name)`, `entries()` (the whole
`(name, dtype)` list by reference), `payloads()` (the dtype strings alone,
value-owned), range-based iteration, and its own `operator<<` for a quick printout.
`ptypes()` returns the *same* object with coarser type strings; its page carries
the [full `DTypes` member reference](ptypes.md).

## Parameters

`None`

## Returns

`DTypes`
    An ordered `(feature name, dtype string)` view — one entry per column, in the
    frame's feature order. Iterate it for pairs, look one up with `at("name")` /
    `operator[]`, take the whole list with `entries()`, or take just the dtype
    strings (value-owned) with `payloads()`. Full member reference on the
    [`ptypes()` page](ptypes.md).

## Examples

Example 1. Inspect concrete dtypes before deciding whether to cast anything.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed",     pluss::make_series({1.0, 0.0, 1.0})},
    {"raw_name",   pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto out = raw.dtypes();
std::cout << out;
```

### Output

```text
features  dtype
--------  --------
student_id  Series<float64>
passed  Series<float64>
raw_name  Note<utf8>
```

Both `student_id` and `passed` came in as `Series<float64>` even though `passed` clearly
holds a 0/1 flag — the loader/constructor did not infer `bool` on its own. That is
precisely the kind of thing `dtypes()` exists to surface before you commit to a
computation: follow up with
[`astype({"passed"}, pluss::ScalarType::Bool)`](astype.md) once you have confirmed it
from this output, rather than assuming the flag column is already boolean.

Note the row alignment above is not a typo: `DTypes::operator<<` prints a fixed
`"features  dtype"` header and a fixed-width separator, then each `name  dtype` pair with
a plain two-space gap and no further column padding — unlike `DataFrame`'s own renderer,
this printout does not widen its columns to fit the longest name.

Example 2. Look up one feature's dtype directly, without iterating everything.

```cpp
auto out = raw.dtypes();
if (out.contains("passed")) {
  std::cout << out["passed"] << "\n";  // "Series<float64>"
}
```

## Notes

- `dtypes()` reports the *concrete* dtype (`Series<float64>`, `Note<utf8>`,
  `Timeline<datetime64[ns, UTC]>`, `TimeIndex<datetime64[ns, UTC]>`). Use
  [`ptypes()`](ptypes.md) instead when you only need the coarse payload family without
  the concrete scalar type.
- `TimeIndex` is intentionally reported separately from `Timeline`: a `Timeline` stores
  temporal *values*, while a `TimeIndex` is the temporal *axis* payload used for
  time-label alignment and resampling — they are different roles even when the
  underlying concrete type looks similar.
- Iteration order matches the frame's current feature order, the same order
  [`features()`](features.md) returns.

## See Also

- [`DataFrame::ptypes`](ptypes.md): the coarse payload family only.
- [`DataFrame::cast`](cast.md), [`DataFrame::astype`](astype.md): change dtypes after
  inspecting them here.
- [`DataFrame::info`](info.md): dtypes alongside null counts, case range, and memory in
  one combined report.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
