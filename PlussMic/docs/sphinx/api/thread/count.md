# Thread::count

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::count(bool count_true = true) -> ThreadValue
```

Count entries in the current thread.

For ordinary numeric, text, or temporal payloads, `count(true)` is the non-missing count. For boolean series, `count(true)` and `count(false)` let you count each truth state explicitly.

## Parameters

**count_true**

Boolean target to count on boolean series payloads.

- `true`: count `true` values.
- `false`: count `false` values.

## Returns

`ThreadValue`
    Scalar-like count result.

## Notes

`count(false)` is mainly useful for boolean series payloads.

## Examples

Example 1. Count how many `true` values appear in a boolean series.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true, true}));

auto out = flags.count(true);
```

### Output

```text
3
```

Example 2. Count how many `false` values appear in a boolean series.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true, true}));

auto out = flags.count(false);
```

### Output

```text
1
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr_with.html">
    <span class="label">Previous</span>
    <span class="title">corr_with</span>
  </a>
  <a class="next-link" href="covar.html">
    <span class="label">Next</span>
    <span class="title">covar</span>
  </a>
</div>
