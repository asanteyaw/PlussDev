# DataFrame::dropnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::dropnull(const DropOptions& options) -> DataFrame

DataFrame::dropnull(pluss::RangeView<std::string_view> features = {}, bool all = false) -> DataFrame
```

Drop rows according to a missing-data rule.

This is the row-removal counterpart to `fillna(...)` and `mask(...)`.

Use it when rows with incomplete measurements, grades, or join keys should be excluded from later computation.

## Parameters

**features**

Selected features to inspect for missingness.

Leave this empty to inspect every feature. Pass a subset when only certain grading, measurement, or key columns should decide whether a row stays.

**all**

Row-drop rule for the selected features.

- `false`: drop a row when *any* selected feature is missing,
- `true`: drop a row only when *all* selected features are missing.

This is the main behavioral toggle. It decides whether the rule is strict (`any missing => drop`) or lenient (`drop only if everything inspected is missing`).

**options**

Structured row-drop policy.

Use `DropOptions` when you want the explicit field names together with additional controls such as:
- `axis` for row-versus-feature dropping routes,
- `all` for all-missing versus any-missing logic,
- `threshold` for minimum non-missing counts.

Reach for this overload when the short overload is no longer expressive enough and you want the null policy stated field by field in user code.

## Returns

`DataFrame`
    Frame containing only the rows that survive the requested null policy.

## See Also

- <a href="fillna.html"><code>DataFrame::fillna</code></a>: replace missing values instead of dropping rows.
- <a href="isnull.html"><code>DataFrame::isnull</code></a>, <a href="notnull.html"><code>DataFrame::notnull</code></a>: inspect missingness first.

## Notes

The simple overload is the common “drop rows with missing values in these columns” route.

`DropOptions` is the more explicit route when you want the null policy written out field by field.

The output preserves the original row order of every surviving case.

## Examples

Example 1. Drop rows when any selected grading column is missing.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"quiz", pluss::make_series({10.0, pluss::nan(), 8.0, pluss::nan()})},
    {"final", pluss::make_series({80.0, 75.0, pluss::nan(), 90.0})},
});

auto out = missing.dropnull({"quiz", "final"}, false);
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80
---------------------------
[1 cs x 3 feat]
```

Example 2. Spell the same rule through `DropOptions`.

```cpp
using namespace pluss::table;

auto missing = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"quiz", pluss::make_series({10.0, pluss::nan(), 8.0, pluss::nan()})},
    {"final", pluss::make_series({80.0, 75.0, pluss::nan(), 90.0})},
});

DataFrame::DropOptions options;
options.features = {"quiz", "final"};
options.axis = Axis::Case;
options.all = false;

auto out = missing.dropnull(options);
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80
---------------------------
[1 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_levels.html">
    <span class="label">Previous</span>
    <span class="title">drop_levels</span>
  </a>
  <a class="next-link" href="dtypes.html">
    <span class="label">Next</span>
    <span class="title">dtypes</span>
  </a>
</div>
