# DataFrame::fold

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::fold(const std::function<Thread(const DataFrame&)>& fn, ResultShape shape = ResultShape::Reduce) -> Thread
```

## What It Does

Reduces rows into one or more summary values using a folding-style aggregation route.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**shape**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

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

auto out = df.fold();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
