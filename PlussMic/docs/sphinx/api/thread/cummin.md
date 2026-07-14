# Thread::cummin

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::cummin(bool skip_nulls = true) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**skip_nulls**
    When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `cummin` to the current thread using explicit `skip_nulls` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.cummin(true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummax.html">
    <span class="label">Previous</span>
    <span class="title">cummax</span>
  </a>
  <a class="next-link" href="cumprod.html">
    <span class="label">Next</span>
    <span class="title">cumprod</span>
  </a>
</div>
