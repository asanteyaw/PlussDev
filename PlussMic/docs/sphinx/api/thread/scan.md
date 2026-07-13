# Thread::scan

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::scan(ScanStatistic statistic, bool skip_nulls = true) -> Thread
```

## What It Does

Computes a cumulative statistic such as running sum, product, min, or max down the thread.

## Parameters

**statistic**
    Supported values:
    - `Sum`: Compute a cumulative sum down the feature.
    - `Product`: Compute a cumulative product down the feature.
    - `Minimum`: Compute the running minimum down the feature.
    - `Maximum`: Compute the running maximum down the feature.

**skip_nulls**
    When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `scan` to the current thread using explicit `statistic, skip_nulls` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.scan({}, true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="se.html">
    <span class="label">Next</span>
    <span class="title">se</span>
  </a>
</div>
