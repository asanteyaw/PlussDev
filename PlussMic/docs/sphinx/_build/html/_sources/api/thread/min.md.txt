# Thread::min

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::min() -> ThreadValue
```

## What It Does

Returns the minimum value of a numeric thread.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `min` and inspect the resulting summary value or summary object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.min();
```

### Output

```text
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="median.html">
    <span class="label">Previous</span>
    <span class="title">median</span>
  </a>
  <a class="next-link" href="mod.html">
    <span class="label">Next</span>
    <span class="title">mod</span>
  </a>
</div>
