# DataFrame::rpow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rpow(double other) -> DataFrame
```

## What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rpow(10);
```

### Output

```text
a      b
10.0   100.0
100.0  10000.0
1000.0 100000.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="rsub.html">
    <span class="label">Next</span>
    <span class="title">rsub</span>
  </a>
</div>
