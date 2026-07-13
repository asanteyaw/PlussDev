# DataFrame::group_by

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::group_by(const CaseGroup& keys, pluss::RangeView<GroupSpec> specs, bool dropna_keys = true, bool render = false) -> DataFrameView

DataFrame::group_by(pluss::RangeView<std::string_view> keys, pluss::RangeView<GroupSpec> specs, bool dropna_keys = true, bool render = false) -> DataFrameView

DataFrame::group_by(std::initializer_list<std::string_view> keys, pluss::RangeView<GroupSpec> specs, bool dropna_keys = true, bool render = false) -> DataFrameView
```

## What It Does

Groups rows by one or more key features and computes the requested aggregations for each group.

## Parameters

**keys**
    Feature names whose distinct value combinations define the output groups.

**specs**
    Grouped aggregation specs. Each `GroupSpec` has `output`, `feature`, and `op`.

    `op` supports:
    - `Count`: Count observed values inside each group.
    - `Nunique`: Count distinct observed values inside each group.
    - `Sum`: Compute the numeric sum inside each group.
    - `Mean`: Compute the arithmetic mean inside each group.
    - `Min`: Compute the minimum inside each group.
    - `Max`: Compute the maximum inside each group.
    - `Std`: Compute the sample standard deviation inside each group.
    - `Var`: Compute the sample variance inside each group.

**dropna_keys**
    When `true`, rows with null pivot/group keys are excluded before reshaping.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

### Output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```
Example 2. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

### Output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```
Example 3. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

### Output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```

## Execution Model

- `group_by(...)` factorizes typed key combinations once in stable first-seen order.
- Count, sum, mean, minimum, maximum, sample standard deviation, and sample variance use vectorized Series scatter reductions.
- Numeric payloads and validity masks stay on their source platform; key hashing and arbitrary-value distinct counting require one bulk host materialization.
- Output key features retain their original Thread kind and dtype.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
