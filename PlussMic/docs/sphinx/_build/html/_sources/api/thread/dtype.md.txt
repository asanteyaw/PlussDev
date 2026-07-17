# Thread::dtype

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::dtype() -> std::string
```

Return the concrete dtype name used by the underlying thread payload.

This is the concrete readout, not the higher-level payload-family summary used by `Thread::ptype()` and `DataFrame::ptypes()`.

Use it when you care about concrete storage/readout detail rather than only the broad payload family.

## Parameters

`None`

## Returns

`std::string`
    Concrete dtype name such as `float64`, `int64`, `bool`, `utf8`, or `datetime64[ns, UTC]`.

## See Also

- <a href="ptype.html"><code>Thread::ptype</code></a>: inspect the broader payload family.
- <a href="../dataframe/dtypes.html"><code>DataFrame::dtypes</code></a>: inspect concrete feature dtypes across an entire frame.

## Notes

`dtype()` is most informative after parsing or casting numeric data, when you need to confirm whether the thread is currently carrying `float64`, `int64`, `bool`, or another scalar representation.

## Examples

Example 1. Inspect the scalar dtype of a numeric thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_series({10.0, 20.0, 30.0})).dtype();
```

### Output

```text
float64
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
