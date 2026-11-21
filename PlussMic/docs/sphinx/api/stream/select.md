<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 58047a1f7d81 -->

# Stream::select

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
static Stream Stream::select(
    pluss::RangeView<pluss::table::Shelf> conditions,
    pluss::RangeView<Stream> choices,
    const Stream& default_value)

static Stream Stream::select(
    std::initializer_list<pluss::table::Shelf> conditions,
    std::initializer_list<Stream> choices,
    const Stream& default_value)
```

## What It Does

Chooses row-wise between whole streams, using boolean `Shelf` masks as the conditions.

Payload selection delegates to [`Shelf::select`](../shelf/select.md). If the streams
carry labels, label selection is delegated a second time on the label axis so the output
keeps the matching row labels too.

## Notes

- `conditions.size()` must equal `choices.size()`.
- All conditions must be boolean `Shelf` masks of the same length.
- All choices and the default must have matching payload kind.
- If any choice carries labels, all choices and the default must carry labels.

## See Also

- [`Shelf::select`](../shelf/select.md): the lower-level row-wise chooser this delegates
  to.
- [`Catalog::select`](../catalog/select.md): the whole-catalog coordinator built on this.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scribbelize.html">
    <span class="label">Previous</span>
    <span class="title">scribbelize</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
