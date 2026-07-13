# DataFrame::se

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::se(Axis axis = Axis::Case, ErrorTarget target = ErrorTarget::Mean, const StatisticOptions& options = {}) -> Thread
```

## What It Does

Computes row-wise or column-wise standard error summaries for numeric data.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**target**
    Target payload family chosen by this cast overload.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.se();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
