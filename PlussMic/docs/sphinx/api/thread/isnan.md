# Thread::isnan

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::isnan() -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `isnan` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnan();
```

### Output

```text
[true, false, true]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isfinite.html">
    <span class="label">Previous</span>
    <span class="title">isfinite</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
