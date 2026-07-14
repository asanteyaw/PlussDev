# Thread::mode

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::mode(bool drop_nulls = true) -> Thread
```

## What It Does

Returns the most frequently observed value in the thread.

## Parameters

**drop_nulls**
    When `true`, missing values do not participate while the most-frequent value is selected.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `mode` to the current thread using explicit `drop_nulls` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.mode(true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mod.html">
    <span class="label">Previous</span>
    <span class="title">mod</span>
  </a>
  <a class="next-link" href="month.html">
    <span class="label">Next</span>
    <span class="title">month</span>
  </a>
</div>
