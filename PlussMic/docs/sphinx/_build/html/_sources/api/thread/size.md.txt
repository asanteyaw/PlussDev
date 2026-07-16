# Thread::size

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::size() -> std::int64_t
```

## What It Does

Returns a size/count summary for the current object without changing it.

## Parameters

`None`

## Returns

`std::int64_t`
    Integer count, size, or positional result.

## Examples

Example 1. Inspect the `size` property of the current object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.size();
```

### Output

```text
3
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shuffle.html">
    <span class="label">Previous</span>
    <span class="title">shuffle</span>
  </a>
  <a class="next-link" href="skew.html">
    <span class="label">Next</span>
    <span class="title">skew</span>
  </a>
</div>
