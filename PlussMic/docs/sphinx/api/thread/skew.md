# Thread::skew

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::skew(std::int64_t min_count = 3) -> ThreadValue
```

## What It Does

Returns sample skewness of a numeric thread.

## Parameters

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `skew` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.skew(2);
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
