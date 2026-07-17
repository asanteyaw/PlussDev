# Thread::ptype

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ptype() -> std::string
```

Return the broad Pluss payload family stored by this thread.

This is the 1D equivalent of `DataFrame::ptypes()`.

## Parameters

`None`

## Returns

`std::string`
    Payload family name: `Series`, `Note`, `Timeline`, or `TimeIndex`.

## See Also

- <a href="dtype.html"><code>Thread::dtype</code></a>: inspect the concrete payload dtype.
- <a href="../dataframe/ptypes.html"><code>DataFrame::ptypes</code></a>: inspect payload families across a frame.

## Notes

`ptype()` is for dispatching by payload family. Use `dtype()` when storage detail matters.

## Examples

Example 1. Inspect a numeric thread's payload family.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_series({10.0, 20.0, 30.0})).ptype();
```

### Output

```text
Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="put.html">
    <span class="label">Next</span>
    <span class="title">put</span>
  </a>
</div>
