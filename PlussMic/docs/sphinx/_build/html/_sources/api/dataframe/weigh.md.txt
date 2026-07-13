# DataFrame::weigh

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::weigh(pluss::RangeView<double> weights, WindowStatistic statistic = WindowStatistic::Mean, std::int64_t min_count = 1) -> DataFrame
```

## What It Does

Computes weighted trailing window statistics over numeric features using an explicit weight vector.

## Parameters

**weights**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**statistic**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.weigh();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
