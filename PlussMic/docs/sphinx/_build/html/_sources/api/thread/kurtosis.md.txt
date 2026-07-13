# Thread::kurtosis

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::kurtosis(std::int64_t min_count = 4) -> ThreadValue
```

## What It Does

Computes sample kurtosis of a numeric thread.

## Parameters

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `kurtosis` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.kurtosis(2);
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
