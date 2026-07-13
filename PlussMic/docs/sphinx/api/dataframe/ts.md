# DataFrame::ts

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ts(pluss::RangeView<std::string_view> features = {}) -> DataFrameTsAccessor

DataFrame::ts(std::initializer_list<std::string_view> features) -> DataFrameTsAccessor

DataFrame::ts(std::string_view feature) -> DataFrameTsAccessor
```

## What It Does

Returns the time-series accessor bound to one or more selected features.

## Parameters

**features**
    Feature names to bind into the returned time-series accessor.

**feature**
    Feature name to bind into the returned time-series accessor.

## Returns

`DataFrameTsAccessor`
    Value produced by this call.

## Examples

Example 1. Bind the time-series accessor to a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

### Output

```text
DataFrameTsAccessor(features=["load"])
```
Example 2. Bind the time-series accessor by passing several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

### Output

```text
DataFrameTsAccessor(features=["load"])
```
Example 3. Bind the time-series accessor to one feature.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

### Output

```text
DataFrameTsAccessor(features=["load"])
```

## Practical Behavior

- `ts(...)` does not materialize a new DataFrame by itself; it returns the time-series accessor bound to the selected features.
- Use the accessor when you want downstream resampling, lag blocks, rolling blocks, or calendar features on those bound features.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="truediv.html">
    <span class="label">Previous</span>
    <span class="title">truediv</span>
  </a>
  <a class="next-link" href="tz_convert.html">
    <span class="label">Next</span>
    <span class="title">tz_convert</span>
  </a>
</div>
