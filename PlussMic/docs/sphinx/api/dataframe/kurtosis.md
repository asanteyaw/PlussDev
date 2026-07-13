# DataFrame::kurtosis

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::kurtosis(Axis axis = Axis::Case, std::int64_t min_count = 4) -> Thread
```

## What It Does

Computes row-wise or column-wise sample kurtosis for numeric data.

## Parameters

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kurtosis();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="lengthen.html">
    <span class="label">Next</span>
    <span class="title">lengthen</span>
  </a>
</div>
