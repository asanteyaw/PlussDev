# Thread::clear_caselbl_

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clear_caselbl_() -> Thread&
```

Remove explicit case labels from the current thread in place.

Use `Thread::clear_caselbl_()` when the payload should keep its values but the label removal should mutate the current thread object.

## Parameters

`None`

## Returns

`Thread&`
    Reference to the same thread after its explicit case labels have been removed.

## Notes

Only the label axis is cleared. The payload values and validity mask remain unchanged.

## Examples

Example 1. Clear thread labels in place.

```cpp
using namespace pluss::table;

auto thread = Thread(pluss::make_series({10.0, 20.0, 30.0}))
    .caselbl(Thread(pluss::make_note({"row0", "row1", "row2"})));

thread.clear_caselbl_();
```

### Output

```text
[10, 20, 30]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_caselbl.html">
    <span class="label">Previous</span>
    <span class="title">clear_caselbl</span>
  </a>
  <a class="next-link" href="clear_display_name.html">
    <span class="label">Next</span>
    <span class="title">clear_display_name</span>
  </a>
</div>
