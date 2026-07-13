# DataFrame::rolling

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rolling(pluss::RangeView<WindowSpec> specs, bool keep_input = true) -> DataFrame
```

## What It Does

Computes rolling-window features and returns them as a derived frame.

## Parameters

**specs**
    Rolling feature specs. Each `WindowSpec` has these fields.

    Supported values:
    - ``output``: Name of the derived feature to create.
    - ``feature``: Source numeric feature to read from.
    - ``op``: Rolling statistic such as `RollingMean`, `RollingSum`, `RollingMin`, or `RollingMax`.
    - ``window``: Fixed trailing window width in cases.
    - ``min_periods``: Minimum observed values required before the output becomes valid.

**keep_input**
    When `true`, the original features stay in the output and rolling features are appended. When `false`, only the derived rolling outputs are returned.

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

auto out = df.rolling({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, false);
```

### Output

```text
sales_roll
100.0
110.0
105.0
```

## Practical Behavior

- `rolling(...)` is the frame-level window-engineering route, so one call can derive several rolling outputs at once.
- `keep_input=true` appends rolling outputs to the original frame, while `keep_input=false` returns only the engineered outputs.
- Each `WindowSpec` controls the output name, source feature, operation, window width, and minimum observation rule.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rmul.html">
    <span class="label">Previous</span>
    <span class="title">rmul</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
