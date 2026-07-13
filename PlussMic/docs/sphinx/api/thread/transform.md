# Thread::transform

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::transform(const std::function<Thread(const Thread&)>& fn, ResultShape shape = ResultShape::Reduce) -> Thread

Thread::transform(const std::function<ThreadValue(const ThreadValue&)>& fn) -> Thread
```

## What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**shape**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.transform(other, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply a user function and inspect the transformed result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.transform({});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
