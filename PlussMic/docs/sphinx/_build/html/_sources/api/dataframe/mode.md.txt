# DataFrame::mode

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::mode(bool drop_nulls = true) -> DataFrame
```

## What It Does

Returns the most frequently observed value for each compatible feature.

## Parameters

**drop_nulls**
    When `true`, missing values do not participate while the most-frequent value is selected.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Find the most frequent observed value in each compatible feature.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::table::Thread(pluss::make_note({"North", "North", "West", "North"}))},
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Esi", "Kojo"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 88.0, 84.0, 91.0}))},
});

auto out = df.mode(true);
```

### Output

```text
id  sales  city
1   90.0   Accra
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mod.html">
    <span class="label">Previous</span>
    <span class="title">mod</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
