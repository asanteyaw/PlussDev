# Thread::clear_caselbl

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clear_caselbl() -> Thread
```

Return a copy of the current thread with explicit case labels removed.

Use `Thread::clear_caselbl()` when the payload values should stay the same but the thread should go back to unlabeled positional semantics.

## Parameters

`None`

## Returns

`Thread`
    A copy of the thread with its explicit case-label axis removed.

## Notes

This does not change the payload values, dtype, or validity mask.

If the thread already has no explicit case labels, the returned copy is structurally equivalent to the input.

## Examples

Example 1. Drop explicit thread labels while keeping the values.

```cpp
using namespace pluss::table;

auto thread = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .caselbl(Thread(pluss::make_note({"row0", "row1", "row2"})));

auto out = thread.clear_caselbl();
```

### Output

```text
[10, 20, 30]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="clear_caselbl_.html">
    <span class="label">Next</span>
    <span class="title">clear_caselbl_</span>
  </a>
</div>
