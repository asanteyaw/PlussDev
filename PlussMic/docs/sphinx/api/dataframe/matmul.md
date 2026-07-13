# DataFrame::matmul

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::matmul(const DataFrame& other) -> DataFrame
```

## What It Does

Treats the numeric part of the frame as a matrix and returns the matrix product against another numeric frame.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Multiply the numeric matrix represented by this frame with another numeric frame.

```cpp
auto left = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0}))},
    {"b", pluss::table::Thread(pluss::make_series({3.0, 4.0}))},
});
auto right = pluss::table::DataFrame({
    {"c", pluss::table::Thread(pluss::make_series({5.0, 6.0}))},
    {"d", pluss::table::Thread(pluss::make_series({7.0, 8.0}))},
});

auto out = left.matmul(right);
```

### Output

```text
c     d
23.0  31.0
34.0  46.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match.html">
    <span class="label">Previous</span>
    <span class="title">match</span>
  </a>
  <a class="next-link" href="matrix.html">
    <span class="label">Next</span>
    <span class="title">matrix</span>
  </a>
</div>
