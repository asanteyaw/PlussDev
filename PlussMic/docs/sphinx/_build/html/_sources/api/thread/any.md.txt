# Thread::any

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::any(bool skipna = true) -> ThreadValue
```

Return whether at least one value in a boolean thread is true.

`Thread::any(...)` is the scalar existence test for one boolean thread. Use it when one truthy element is enough to satisfy the condition.

## Parameters

**skipna**

Missing-value policy for the reduction.

- `true`: ignore missing boolean positions while reducing.
- `false`: missing boolean payloads are not supported for this reduction path.

## Returns

`ThreadValue`
    One scalar truth result.

## See Also

- <a href="all.html"><code>Thread::all</code></a>: require every element to be true instead of only one.
- <a href="../dataframe/any.html"><code>DataFrame::any</code></a>: apply the same idea across a frame.

## Notes

This method expects a boolean `Series` payload. It is not a general numeric reduction.

Nullable boolean input requires `skipna=true`.

## Examples

Example 1. Reduce one boolean thread to a single truth result.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true}));

auto out = flags.any();
```

### Output

```text
true
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="append.html">
    <span class="label">Next</span>
    <span class="title">append</span>
  </a>
</div>
