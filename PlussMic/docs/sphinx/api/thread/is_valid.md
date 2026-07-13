# Thread::is_valid

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::is_valid(std::int64_t row) -> bool
```

## What It Does

Checks whether the value at one position is marked valid by the thread validity mask.

## Parameters

**row**
    Zero-based row position used by this overload.

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.is_valid(0);
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="interp.html">
    <span class="label">Previous</span>
    <span class="title">interp</span>
  </a>
  <a class="next-link" href="isfinite.html">
    <span class="label">Next</span>
    <span class="title">isfinite</span>
  </a>
</div>
