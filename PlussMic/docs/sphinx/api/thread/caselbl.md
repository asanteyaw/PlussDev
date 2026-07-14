# Thread::caselbl

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::caselbl() -> Thread

Thread::caselbl(const Thread& labels) -> Thread
```

Read or replace the case-label axis attached to a Thread.

`Thread::caselbl()` is the thread-level label accessor. Use it when you want the current labels themselves, and use the overload that accepts `labels` when you want a relabeled copy of the thread.

## Parameters

`None`

## Returns

`Thread`
    For the getter overload, a thread containing the current case labels.

    For the setter overload, a relabeled copy whose payload values and validity mask are preserved.

## See Also

- <a href="clear_caselbl.html"><code>Thread::clear_caselbl</code></a>: remove explicit case labels from a thread.
- <a href="align.html"><code>Thread::align</code></a>: reorder one thread to a supplied label axis.

## Notes

`caselbl()` is about the label axis only. It does not change the payload values.

When you pass `labels`, the replacement label thread must have the same size as the current thread.

## Examples

Example 1. Inspect the current thread case labels.

```cpp
using namespace pluss::table;

auto thread = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .caselbl(Thread(pluss::make_note({"row0", "row1", "row2"})));

auto out = thread.caselbl();
```

### Output

```text
0  row0
1  row1
2  row2
dtype: Note
```

Example 2. Return a relabeled copy of the same thread.

```cpp
using namespace pluss::table;

auto thread = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .caselbl(Thread(pluss::make_note({"row0", "row1", "row2"})));

auto labels = Thread(pluss::make_note({"A", "B", "C"}));
auto out = thread.caselbl(labels);
```

### Output

```text
thread.caselbl():
0  row0
1  row1
2  row2
dtype: Note

out.caselbl():
0  A
1  B
2  C
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="caselbl_.html">
    <span class="label">Next</span>
    <span class="title">caselbl_</span>
  </a>
</div>
