# Thread::prod

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::prod() -> ThreadValue
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `prod` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.prod();
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pow.html">
    <span class="label">Previous</span>
    <span class="title">pow</span>
  </a>
  <a class="next-link" href="ptype.html">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>
