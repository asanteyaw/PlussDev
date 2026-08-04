<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ac6479ef133 -->

# Stream::select

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
static Stream Stream::select(
    pluss::msharp::RangeView::Of<pluss::table::Thread> conditions,
    pluss::msharp::RangeView::Of<Stream> choices,
    const Stream& default_value)

static Stream Stream::select(
    std::initializer_list<pluss::table::Thread> conditions,
    std::initializer_list<Stream> choices,
    const Stream& default_value)
```

## What It Does

Chooses row-wise between whole streams, using boolean `Thread` masks as the conditions.

Payload selection delegates to [`Thread::select`](../thread/select.md). If the streams
carry labels, label selection is delegated a second time on the label axis so the output
keeps the matching row labels too.

## Notes

- `conditions.size()` must equal `choices.size()`.
- All conditions must be boolean `Thread` masks of the same length.
- All choices and the default must have matching payload kind.
- If any choice carries labels, all choices and the default must carry labels.

## See Also

- [`Thread::select`](../thread/select.md): the lower-level row-wise chooser this delegates
  to.
- [`Catalog::select`](../catalog/select.md): the whole-catalog coordinator built on this.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
