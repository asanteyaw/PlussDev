# Thread::percentiles

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::percentiles(pluss::RangeView<double> points = {0.25, 0.5, 0.75}, std::int64_t min_count = 1) -> Thread
```

## What It Does

Computes requested percentile points for a numeric thread.

## Parameters

**points**
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `percentiles` to the current thread using explicit `points, min_count` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.percentiles({}, 2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="partition.html">
    <span class="label">Previous</span>
    <span class="title">partition</span>
  </a>
  <a class="next-link" href="pow.html">
    <span class="label">Next</span>
    <span class="title">pow</span>
  </a>
</div>
