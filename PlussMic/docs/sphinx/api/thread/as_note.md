# Thread::as_note

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_note() -> const pluss::Note&
```

Expose the underlying `pluss::Note` payload directly.

`Thread::as_note()` is the escape hatch you use when `Thread` itself intentionally stays smaller than the full text-method surface. Once you have the `Note`, you can call Note-specific text operations and then wrap the result back into `Thread` only when a thread-aware API needs it again.

## Parameters

`None`

## Returns

`const pluss::Note&`
    Reference to the underlying note payload.

## See Also

- <a href="as_series.html"><code>Thread::as_series</code></a>: expose the numeric payload directly.
- <a href="as_timeline.html"><code>Thread::as_timeline</code></a>: expose the temporal payload directly.
- <a href="as_timeindex.html"><code>Thread::as_timeindex</code></a>: expose the indexed temporal payload directly.

## Notes

This method does not create a new thread by itself. It returns the underlying payload reference.

If a downstream API expects `Thread`, wrap the transformed payload back as `Thread(note_result)`.

## Examples

Example 1. Use the Note payload to run text cleanup and then wrap it back into a Thread.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({" Ada ", "BOLA", " chen "}));

auto out = Thread(names.as_note().strip().lower());
```

### Output

```text
0  ada
1  bola
2  chen
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="append.html">
    <span class="label">Previous</span>
    <span class="title">append</span>
  </a>
  <a class="next-link" href="as_series.html">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>
