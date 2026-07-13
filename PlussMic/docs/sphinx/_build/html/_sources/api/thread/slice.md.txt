# Thread::slice

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::slice(std::int64_t start, std::int64_t stop) -> Thread
```

## What It Does

Returns a contiguous slice `[start, stop)` of the thread.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.slice(0, 2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skew.html">
    <span class="label">Previous</span>
    <span class="title">skew</span>
  </a>
  <a class="next-link" href="sort_values.html">
    <span class="label">Next</span>
    <span class="title">sort_values</span>
  </a>
</div>
