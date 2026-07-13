# Thread::window

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::window(std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) -> Thread
```

## What It Does

Computes a window-based transformation such as a rolling mean over the thread values.

## Parameters

**window**
    Positive fixed case count in each moving window.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

**min_periods**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `window` to the current thread using explicit `window, op, min_periods` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.window(2, pluss::table::ThreadWindowOp::RollingMean, 1);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weigh.html">
    <span class="label">Previous</span>
    <span class="title">weigh</span>
  </a>
  <a class="next-link" href="with_display_name.html">
    <span class="label">Next</span>
    <span class="title">with_display_name</span>
  </a>
</div>
