# Thread::flow

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::flow(const std::function<Thread(const Thread&)>& fn) -> Thread
```

## What It Does

Computes a cumulative directional summary across the thread values.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `flow` to the current thread using explicit `fn` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.flow(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="fold.html">
    <span class="label">Next</span>
    <span class="title">fold</span>
  </a>
</div>
