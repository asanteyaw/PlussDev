# Thread::dropnull

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::dropnull() -> Thread
```

Remove entries marked missing from the thread.

This is the direct thread-level route when you want only the observed values carried forward.

## Parameters

`None`

## Returns

`Thread`
    Thread containing only the non-missing entries.

## Notes

`dropnull()` operates on the thread validity mask. Plain `NaN` values inside a numeric series are not dropped unless they are represented as missing by the thread mask.

The result is compacted: surviving entries are returned as a new shorter thread rather than leaving null holes in place.

## Examples

Example 1. Drop entries masked as missing.

```cpp
using namespace pluss::table;

auto values = Thread(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, false})
);

auto out = values.dropnull();
```

### Output

```text
0  10
1  30
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
