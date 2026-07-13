# Thread::shape

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shape() -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shape();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="shares_payload_with.html">
    <span class="label">Next</span>
    <span class="title">shares_payload_with</span>
  </a>
</div>
