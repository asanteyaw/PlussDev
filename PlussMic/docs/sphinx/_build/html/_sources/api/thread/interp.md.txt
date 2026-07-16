# Thread::interp

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::interp(std::int64_t limit = -1) -> Thread
```

## What It Does

Interpolates missing numeric values inside the thread.

## Parameters

**limit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `interp` to the current thread using explicit `limit` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.interp(2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info_string.html">
    <span class="label">Previous</span>
    <span class="title">info_string</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
