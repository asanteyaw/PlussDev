# Thread::freq

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::freq(std::int64_t top_n = 0, bool dropnull = true) -> Thread
```

## What It Does

Counts the most frequent observed values in the thread.

## Parameters

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

**dropnull**
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `freq` to the current thread using explicit `top_n, dropnull` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.freq(2, true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fold.html">
    <span class="label">Previous</span>
    <span class="title">fold</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
