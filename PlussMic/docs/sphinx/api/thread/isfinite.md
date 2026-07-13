# Thread::isfinite

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::isfinite() -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `isfinite` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isfinite();
```

### Output

```text
[true, false, true]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_valid.html">
    <span class="label">Previous</span>
    <span class="title">is_valid</span>
  </a>
  <a class="next-link" href="isnan.html">
    <span class="label">Next</span>
    <span class="title">isnan</span>
  </a>
</div>
