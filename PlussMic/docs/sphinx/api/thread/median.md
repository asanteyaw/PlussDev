# Thread::median

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::median() -> ThreadValue
```

## What It Does

Returns the median of a numeric thread.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `median` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.median();
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
