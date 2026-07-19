# Thread::as_timeline

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_timeline() -> const pluss::Timeline&
```

Expose the underlying `pluss::Timeline` payload directly.

`Thread::as_timeline()` is the temporal escape hatch for timeline-specific transforms such as calendar component extraction, localization, rounding, and other specialized chronology operations that belong to the underlying temporal type.

## Parameters

`None`

## Returns

`const pluss::Timeline&`
    Reference to the underlying timeline payload.

## See Also

- <a href="as_timeindex.html"><code>Thread::as_timeindex</code></a>: expose the indexed temporal payload.
- <a href="as_series.html"><code>Thread::as_series</code></a>: switch to the numeric payload when the result has become numeric.

## Notes

Many timeline operations return numeric or temporal pluss payloads directly. Wrap them back into `Thread(...)` when you want to keep working at the table layer afterward.

## Examples

Example 1. Use the Timeline payload to extract the calendar year and wrap the result back into a Thread.

```cpp
using namespace pluss::table;

auto stamps = Thread(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
}));

auto out = Thread(stamps.as_timeline().year());
```

### Output

```text
0  2024
1  2024
2  2024
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_timeindex.html">
    <span class="label">Previous</span>
    <span class="title">as_timeindex</span>
  </a>
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
