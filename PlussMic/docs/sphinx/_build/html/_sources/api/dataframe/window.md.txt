# DataFrame::window

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::window(pluss::RangeView<WindowSpec> specs, bool keep_input = true) -> DataFrame
```

## What It Does

Computes one or more window-based features such as rolling means over selected columns.

## Parameters

**specs**
    Window feature specs. Each `WindowSpec` has these fields.

    Supported values:
    - ``output``: Name of the derived feature to create.
    - ``feature``: Source numeric feature to read from.
    - ``op``: Window statistic such as `RollingMean`, `RollingSum`, `RollingMin`, or `RollingMax`.
    - ``window``: Fixed trailing window width in cases.
    - ``min_periods``: Minimum observed values required before the output becomes valid.

**keep_input**
    When `true`, the original input features are preserved and engineered window outputs are appended. When `false`, only the engineered outputs are returned.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Build one or more derived rolling/window features from structured `WindowSpec` instructions.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.window({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, false);
```

### Output

```text
sales_roll
100.0
110.0
105.0
```

## Practical Behavior

- `window(...)` engineers one or more derived features from rolling/window operations described by `WindowSpec`.
- `keep_input=false` returns only the engineered outputs, which is useful for model-matrix style pipelines.
- `rolling(...)` is the alias that delegates to the same frame-level implementation.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="where.html">
    <span class="label">Previous</span>
    <span class="title">where</span>
  </a>
</div>
