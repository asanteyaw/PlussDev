# DataFrame::to_utc

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::to_utc(pluss::RangeView<std::string_view> features) -> DataFrame
```

## What It Does

Converts selected temporal features into UTC timezone representation.

## Parameters

**features**
    Temporal feature names to convert into UTC timezone representation.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto localized = df.tz_localize({"ts"}, "UTC");
auto out = localized.to_utc({"ts"});
```

### Output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="top.html">
    <span class="label">Next</span>
    <span class="title">top</span>
  </a>
</div>
