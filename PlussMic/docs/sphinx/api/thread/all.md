# Thread::all

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::all(bool skipnull = true) -> ThreadValue
```

Return whether every value in a boolean thread is true.

`Thread::all(...)` is the scalar truth reduction for one thread. It is the right tool when one falsey element should invalidate the whole column-level condition.

## Parameters

**skipnull**

Missing-value policy for the reduction.

- `true`: ignore missing boolean positions while reducing.
- `false`: missing boolean payloads are not supported for this reduction path.

## Returns

`ThreadValue`
    One scalar truth result.

## See Also

- <a href="any.html"><code>Thread::any</code></a>: require only one truthy element instead of all of them.
- <a href="../dataframe/all.html"><code>DataFrame::all</code></a>: apply the same idea across several features or rows.

## Notes

This method expects a boolean `Series` payload. It is not a general numeric reduction.

Nullable boolean input requires `skipnull=true`.

## Examples

Example 1. Reduce one boolean thread to a single truth result.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series(std::vector<bool>{true, false, true}));

auto out = flags.all();
```

### Output

```text
false
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
