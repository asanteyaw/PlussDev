# Thread::round

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::round(std::int64_t decimals = 0) -> Thread
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**decimals**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `round` to the current thread using explicit `decimals` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.round(2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ror.html">
    <span class="label">Previous</span>
    <span class="title">ror</span>
  </a>
  <a class="next-link" href="rpow.html">
    <span class="label">Next</span>
    <span class="title">rpow</span>
  </a>
</div>
