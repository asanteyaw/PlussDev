# Thread::isnull

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::isnull() -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `isnull` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.isnull();
```

### Output

```text
[true, false, true]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnan.html">
    <span class="label">Previous</span>
    <span class="title">isnan</span>
  </a>
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
