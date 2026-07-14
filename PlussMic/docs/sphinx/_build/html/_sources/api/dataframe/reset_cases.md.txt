# DataFrame::reset_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::reset_cases(const ResetCasesOptions& options) -> DataFrame

DataFrame::reset_cases(std::string case_feature = "case") -> DataFrame
```

## What It Does

Materializes the current case axis as a normal feature, then restores the default positional cases `0..n-1`.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**case_feature**
    Name to use for the restored case-axis feature when the current cases are materialized back into ordinary columns.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.set_cases("city", true);
auto out = with_cases.reset_cases("case");
```

### Output

```text
with_cases.caselbl():
["Accra", "Tema", "Kumasi"]

out.caselbl():
[0, 1, 2]

out.head():
id  sales  case
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.set_cases("city", true);
auto out = with_cases.reset_cases("case");
```

### Output

```text
with_cases.caselbl():
["Accra", "Tema", "Kumasi"]

out.caselbl():
[0, 1, 2]

out.head():
id  sales  case
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="reshuffle.html">
    <span class="label">Next</span>
    <span class="title">reshuffle</span>
  </a>
</div>
