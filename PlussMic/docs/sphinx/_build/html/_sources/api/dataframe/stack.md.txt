<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d1114851b47 -->

# DataFrame::stack

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::stack(std::string feature_name = "feature", std::string value_name = "value") const -> DataFrame
```

## What It Does

Melts **every** feature on the frame into long format in one call: each original
case gets one output row per original feature, with a new `feature_name` column
recording which feature that row came from and a new `value_name` column holding
the corresponding stacked value. The original case index (or, if the frame already
has named case labels, every one of those levels) and the new `feature_name` column
together become the result's case-label axis — `stack()` promotes them there rather
than leaving them as ordinary columns, which is what distinguishes it from
[`lengthen()`](lengthen.md) (a more configurable melt that keeps its `id_features`
as plain columns and lets you choose a `value_features` subset instead of always
stacking everything).

Because every feature's values get concatenated together into one `value_name`
column, every feature on the frame must share the same `Thread` kind (and, for
numeric features, the same concrete dtype) — the same constraint
[`lengthen()`](lengthen.md) places on its `value_features`.

## Parameters

**feature_name** (`std::string`, default `"feature"`)

Name of the new column (well, case-label level) recording each output row's source
feature name.

**value_name** (`std::string`, default `"value"`)

Name of the new column holding each output row's stacked value. Must differ from
`feature_name`.

## Returns

`DataFrame`
    A new frame with `ncases() * nfeatures()` rows and a single remaining feature,
    `value_name`. The original case index and `feature_name` are promoted into the
    result's (multi-level) case-label axis rather than kept as ordinary columns.

## Examples

Example 1. Stack a small two-feature frame, then inspect the tidy long-format
result.

```cpp
using namespace pluss::table;

auto monthly = DataFrame({
    {"jan_sales", Thread(pluss::make_series({100.0, 200.0}))},
    {"feb_sales", Thread(pluss::make_series({150.0, 250.0}))},
});
std::cout << monthly;
```

### Output (before)

```text
===========================
     | jan_sales  feb_sales
---------------------------
0    | 100        150      
1    | 200        250      
---------------------------
[2 cs x 2 feat]
```

```cpp
auto stacked = monthly.stack();
std::cout << stacked.label().names().size() << "\n";  // case-label levels: "case", "feature"
std::cout << stacked.features().size() << "\n";        // remaining ordinary feature: "value"
```

### Output

```text
2
1
```

`stacked` no longer has `jan_sales`/`feb_sales` as its features at all — it has a
single feature, `"value"`, with the original row position (`"case"`, `stack()`'s
default level name for a frame with no pre-existing case labels) and the source
feature name (`"feature"`) both promoted into its case-label axis instead.

Example 2. Pull the promoted levels back out as ordinary columns to see the tidy
result plainly.

```cpp
DataFrame::ResetLabelOptions options;
options.names = {"case", "feature"};
auto plain = stacked.label().reset(options);
std::cout << plain;
```

### Output

```text
=============================
     | value  case  feature  
-----------------------------
0    | 100    0     jan_sales
1    | 200    1     jan_sales
2    | 150    0     feb_sales
3    | 250    1     feb_sales
-----------------------------
[4 cs x 3 feat]
```

Four output rows — `2` original cases times `2` original features. Row `0` is
`monthly`'s case `0`, feature `jan_sales`, value `100`; row `2` is case `0`, feature
`feb_sales`, value `150`; and so on. `stack()` concatenates feature-by-feature (every
`jan_sales` row before any `feb_sales` row), not case-by-case.

## Notes

- Every feature on the source frame must share the same `Thread` kind (and, for
  numeric features, the same concrete scalar dtype and platform), since their values
  are concatenated into one `value_name` column.
- If the source frame already has named case labels, every one of those levels is
  carried into the result's case-label axis alongside `feature_name` — not just a
  single default position column.
- Reach for [`lengthen()`](lengthen.md) instead when you want some columns to stay
  as identifying features rather than being folded into the case-label axis, or when
  only a subset of features should be stacked.

## See Also

- [`DataFrame::lengthen`](lengthen.md): the more configurable melt, keeping
  `id_features` as plain columns and letting you choose which features to stack.
- [`DataFrame::unstack`](unstack.md): the (partial) inverse, spreading one
  case-label level back out into columns.
- [`DataFrame::pivot_table`](pivot_table.md): reshapes long data back into a wide
  summary, the complementary direction to this melt.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sql.html">
    <span class="label">Previous</span>
    <span class="title">sql</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
