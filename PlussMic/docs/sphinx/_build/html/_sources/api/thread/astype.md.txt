# Thread::astype

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::astype(pluss::ScalarType dtype) -> Thread
```

Cast a numeric thread to a different scalar dtype.

`Thread::astype(...)` is the direct dtype-conversion surface on `Thread`. Use it when imported data need normalization, flags stored as `0/1` should become booleans, or downstream kernels expect a more specific scalar dtype.

## Parameters

**dtype**

Target scalar dtype.

Common choices include:

- `pluss::ScalarType::Bool`
- `pluss::ScalarType::Int64`
- `pluss::ScalarType::Float64`

Choose the smallest dtype that still supports the downstream operations you plan to run.

## Returns

`Thread`
    A new thread with the requested dtype conversion applied.

## See Also

- <a href="as_series.html"><code>Thread::as_series</code></a>: drop down to the Series payload when you need broader low-level numeric control.
- <a href="../dataframe/astype.html"><code>DataFrame::astype</code></a>: cast several frame features at once.

## Notes

This is the direct scalar-dtype conversion route. It is best for numeric Series-backed threads.

## Examples

Example 1. Convert a 0/1 numeric thread into a boolean thread.

```cpp
using namespace pluss::table;

auto ints = Thread(pluss::make_series({1.0, 0.0, 1.0}));

auto out = ints.astype(pluss::ScalarType::Bool);
```

### Output

```text
0  true
1  false
2  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_timeline.html">
    <span class="label">Previous</span>
    <span class="title">as_timeline</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
