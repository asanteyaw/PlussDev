# Thread::dtype

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::dtype() -> std::string
```

Return the scalar dtype name used by the underlying numeric series payload.

This is the scalar-level type readout, not the higher-level payload-family summary used by `DataFrame::dtypes()`.

Use it when you care about the concrete numeric storage type rather than only the broad payload family.

## Parameters

`None`

## Returns

`std::string`
    Scalar dtype name such as `float64`, `int64`, or `bool`.

## See Also

- <a href="kind.html"><code>Thread::kind</code></a>: inspect the broader payload family.
- <a href="../dataframe/dtypes.html"><code>DataFrame::dtypes</code></a>: inspect feature families across an entire frame.

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
