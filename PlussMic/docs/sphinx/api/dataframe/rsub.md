# DataFrame::rsub

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::rsub(double other) -> DataFrame
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

auto out = df.rsub(10);
```

### Output

```text
a    b
9.0  8.0
8.0  6.0
7.0  5.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rpow.html">
    <span class="label">Previous</span>
    <span class="title">rpow</span>
  </a>
  <a class="next-link" href="rtruediv.html">
    <span class="label">Next</span>
    <span class="title">rtruediv</span>
  </a>
</div>
