<!-- pluss-hand-authored -->

# DataFrame::lengthen

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::lengthen(pluss::RangeView<std::string_view> id_features = {}, pluss::RangeView<std::string_view> value_features = {}, std::string feature_name = "feature", std::string value_name = "value", bool fresh_cases = true) const -> DataFrame
```

## What It Does

Reshapes a wide frame into long form: `id_features` are repeated (tiled) once per
value feature, and every `value_feature` is stacked into two new columns — one
(`feature_name`) recording which source feature each row came from, and one
(`value_name`) holding the actual stacked values. This is the "melt" / "pivot longer"
direction of reshaping; [`pivot()`](pivot.md) / [`pivot_table()`](pivot_table.md)
build wide frames back out of long ones.

## Parameters

**id_features** (`pluss::RangeView<std::string_view>`, default `{}`)

Features to keep as identifiers, repeated unchanged for every value feature's block.

**value_features** (`pluss::RangeView<std::string_view>`, default `{}`)

Features to stack together into one output value column. Leave empty (the default) to
use every feature not already listed in `id_features`. All selected value features
must share the same `Thread` kind — and, for numeric features, the same scalar dtype
and platform — since they end up concatenated into one homogeneous output column.

**feature_name** (`std::string`, default `"feature"`)

Output column name recording which source value feature each row came from.

**value_name** (`std::string`, default `"value"`)

Output column name holding the actual stacked values.

**fresh_cases** (`bool`, default `true`)

- `true` (the default): the output gets a fresh, contiguous `0, 1, 2, ...` case index.
- `false`: the original case labels are repeated once per value-feature block, so a
  case label can appear more than once in the output.

## Returns

`DataFrame`
    A new frame with `id_features.size() + 2` columns and
    `ncases() * value_features.size()` rows (or `ncases() * (nfeatures() -
    id_features.size())` rows when `value_features` is left empty).

## Examples

Example 1. Melt two quarterly sales columns into long form.

```cpp
using namespace pluss::table;

auto wide = DataFrame({
    {"id",        Thread(pluss::make_series({1, 2}))},
    {"q1_sales",  Thread(pluss::make_series({100.0, 150.0}))},
    {"q2_sales",  Thread(pluss::make_series({110.0, 130.0}))},
});
std::cout << wide;
```

### Output (before)

```text
===============================
     | id    q1_sales  q2_sales
-------------------------------
0    | 1     100       110     
1    | 2     150       130     
-------------------------------
[2 cs x 3 feat]
```

```cpp
auto long_form = wide.lengthen({"id"}, {"q1_sales", "q2_sales"}, "quarter", "sales");
std::cout << long_form;
```

### Output (after)

```text
============================
     | id    quarter   sales
----------------------------
0    | 1     q1_sales  100  
1    | 2     q1_sales  150  
2    | 1     q2_sales  110  
3    | 2     q2_sales  130  
----------------------------
[4 cs x 3 feat]
```

`id` is repeated once per value-feature block (`1, 2` for the `q1_sales` block, then
`1, 2` again for the `q2_sales` block); `quarter` records which source column each row
came from; `sales` holds the stacked values themselves, in the same block order.

Example 2. Leave `value_features` empty to stack every non-id feature automatically.

```cpp
auto same_result = wide.lengthen({"id"});
```

Since `q1_sales` and `q2_sales` are the only two features besides `id`, this produces
the identical result as Example 1 — useful when the wide frame's value columns aren't
known by name ahead of time.

## Notes

- Every selected value feature must share one `Thread` kind (and, for numeric data,
  the same scalar dtype and platform); mixing a `Series` value feature with a `Note`
  one in the same call is not supported, since they must concatenate into a single
  homogeneous output column.
- Any validity (null) mask on a stacked value feature is concatenated along with the
  values, so missing cells in the source stay missing in the output.
- `fresh_cases = false` deliberately produces a case axis with repeated labels (one
  repeat per value-feature block) — pass `true` (the default) unless you specifically
  need to trace each output row back to its original case label.

## See Also

- [`DataFrame::stack`](stack.md) / [`DataFrame::unstack`](unstack.md): the
  case-label-driven reshaping methods, an alternative long/wide transformation using
  structured case labels instead of named value columns.
- [`DataFrame::pivot`](pivot.md) / [`DataFrame::pivot_table`](pivot_table.md): the
  inverse direction, rebuilding a wide frame from long form.
- [`DataFrame::transpose`](transpose.md): a different reshape entirely — swapping
  rows and features rather than melting value columns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
