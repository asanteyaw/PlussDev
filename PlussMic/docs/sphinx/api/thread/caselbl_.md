# Thread::caselbl_

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::caselbl_(const Thread& labels) -> Thread&
```

Replace the thread case-label axis in place.

Use `Thread::caselbl_(...)` when the relabeling should mutate the current thread object instead of returning a detached copy.

## Parameters

**labels**

New case-label axis to attach to the current thread.

- Its size must match the thread size exactly.
- It can itself be note-backed, numeric, or temporal.

## Returns

`Thread&`
    Reference to the same thread after its case labels have been replaced.

## Notes

Only the label axis changes. The payload values and validity mask remain the same.

## Examples

Example 1. Relabel the current thread in place.

```cpp
using namespace pluss::table;

auto thread = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto labels = Thread(pluss::make_note({"A", "B", "C"}));

thread.caselbl_(labels);
```

### Output

```text
thread.caselbl():
0  A
1  B
2  C
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="caselbl.html">
    <span class="label">Previous</span>
    <span class="title">caselbl</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
