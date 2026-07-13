# DataFrame::equals

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::equals(const DataFrame& other) -> bool
```

## What It Does

Checks whether two frames have the same structure, labels, and values.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.equals(right);
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="extend_cases.html">
    <span class="label">Next</span>
    <span class="title">extend_cases</span>
  </a>
</div>
