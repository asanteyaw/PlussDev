# Thread::as_series

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_series() -> const pluss::Series&
```

Expose the underlying `pluss::Series` payload directly.

`Thread::as_series()` is the numeric escape hatch when you want the full Series surface rather than the curated Thread surface. It is especially useful for direct arithmetic chains, native Series reductions, or specialized kernels that already live below the table layer.

## Parameters

`None`

## Returns

`const pluss::Series&`
    Reference to the underlying series payload.

## See Also

- <a href="astype.html"><code>Thread::astype</code></a>: keep the operation on the Thread surface when you only need dtype conversion.
- <a href="as_note.html"><code>Thread::as_note</code></a>: switch to the text payload instead of the numeric one.

## Notes

This method returns the underlying payload reference. It does not automatically rewrap the result into `Thread`.

If you want to call a Thread-only method after Series work, wrap the result back as `Thread(series_result)`.

## Examples

Example 1. Use the Series payload for arithmetic, then wrap the result back into a Thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({-2.5, 0.0, 3.5}));

auto out = Thread(scores.as_series() + 5.0);
```

### Output

```text
0  2.5
1  5
2  8.5
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_note.html">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
  <a class="next-link" href="as_timeindex.html">
    <span class="label">Next</span>
    <span class="title">as_timeindex</span>
  </a>
</div>
