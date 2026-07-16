# Thread::invalid

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::invalid() -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `invalid` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.invalid();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="interp.html">
    <span class="label">Previous</span>
    <span class="title">interp</span>
  </a>
  <a class="next-link" href="is_valid.html">
    <span class="label">Next</span>
    <span class="title">is_valid</span>
  </a>
</div>
