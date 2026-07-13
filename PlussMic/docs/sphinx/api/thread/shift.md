# Thread::shift

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shift(std::int64_t periods = 1) -> Thread
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**periods**
    Number of row or time steps to shift forward or backward. Positive values usually move data down or later; negative values move it up or earlier.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `shift` to the current thread using explicit `periods` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shift(2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shares_payload_with.html">
    <span class="label">Previous</span>
    <span class="title">shares_payload_with</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
