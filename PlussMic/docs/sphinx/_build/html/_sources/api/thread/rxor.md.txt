# Thread::rxor

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::rxor(const Thread& other) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `rxor` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.rxor(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rtruediv.html">
    <span class="label">Previous</span>
    <span class="title">rtruediv</span>
  </a>
  <a class="next-link" href="scan.html">
    <span class="label">Next</span>
    <span class="title">scan</span>
  </a>
</div>
