# DataFrame::scan

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::scan(ScanStatistic statistic, bool skip_nulls = true) -> DataFrame
```

## What It Does

Computes cumulative numeric statistics such as running sums or running maxima down each feature.

## Parameters

**statistic**
    Cumulative statistic to compute down each numeric feature.

    Supported values:
    - `Sum`: Running sum from the first row through the current row.
    - `Product`: Running product from the first row through the current row.
    - `Minimum`: Running minimum seen so far.
    - `Maximum`: Running maximum seen so far.

**skip_nulls**
    When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Run one cumulative statistic down each numeric feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.scan(pluss::table::ScanStatistic::Sum, true);
```

### Output

```text
id  sales
1   100.0
3   220.0
6   310.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="se.html">
    <span class="label">Next</span>
    <span class="title">se</span>
  </a>
</div>
