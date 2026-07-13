# Thread::max

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::max() -> ThreadValue
```

## What It Does

Returns the maximum value of a numeric thread.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `max` and inspect the resulting summary value or summary object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.max();
```

### Output

```text
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
