# DataFrame::tabelize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::tabelize(const pluss::kindle::Catalog& catalog) -> DataFrame

DataFrame::tabelize(const pluss::kindle::Catalog& catalog, const pluss::kindle::TabelizePolicy& policy) -> DataFrame
```

## What It Does

Converts a Kindle `Catalog` into a tabular `DataFrame`, preserving stream alignment as rows.

## Parameters

**catalog**
    Catalog payload supplied to this overload.

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Convert a Kindle catalog into a frame with the default tabelization policy.

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
auto out = pluss::table::DataFrame::tabelize(catalog);
```

### Output

```text
sales
10.0
20.0
30.0
```
Example 2. Convert a Kindle catalog into a frame with an explicit tabelization policy.

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
pluss::kindle::TabelizePolicy policy;
auto out = pluss::table::DataFrame::tabelize(catalog, policy);
```

### Output

```text
sales
10.0
20.0
30.0
```

## Practical Behavior

- `tabelize(catalog)` is the explicit bridge from Kindle stream collections into the DataFrame model.
- Aligned streams become DataFrame features, and stream position becomes row position.
- Use the policy overload when you want explicit tabelization rules instead of the defaults.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
