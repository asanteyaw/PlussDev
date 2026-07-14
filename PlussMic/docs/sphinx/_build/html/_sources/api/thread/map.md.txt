# Thread::map

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::map(const std::function<ThreadValue(const ThreadValue&)>& fn) -> Thread
```

## What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.map({});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lxor.html">
    <span class="label">Previous</span>
    <span class="title">lxor</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
