<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6696490763db -->

# StreamLabelAccessor::set

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signatures

```cpp
Stream StreamLabelAccessor::set(const Stream& labels) const
Stream StreamLabelAccessor::set(const pluss::table::Slate& labels) const
```

## What It Does

Attaches `labels` as the bound stream's row labels, returning a new stream.
Both overloads accept the same length-matching label data — as a `Stream`
or a `pluss::table::Slate` — and are otherwise interchangeable.

```{important} make_note(...) is ambiguous here — wrap it explicitly
`pluss::make_note(...)` (and other `make_*` helpers) implicitly convert to
**both** `Stream` and `pluss::table::Slate` — passing one directly to
`set()` fails to compile with an ambiguous-overload error. Wrap it
explicitly in whichever type you mean:
`set(pluss::table::Slate(pluss::make_note({...})))` or
`set(pluss::kindle::Stream(pluss::make_note({...})))`. A pre-built `Stream`
or `Slate` value (not a bare `make_note(...)` call) resolves cleanly with
no wrapping needed.
```

## Parameters

**labels** (`const Stream&` or `const pluss::table::Slate&`)
    The new row labels. Must be the same length as the bound stream.

## Returns

`Stream`
    A new stream with `labels` attached as its row labels; every other
    aspect of the stream (values, mask) is unchanged.

## Examples

Example 1. Attach `Note`-kind labels via the `Slate` overload.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
auto labeled = stream.lbl().set(Slate(pluss::make_note({"a", "b", "c"})));
std::cout << labeled;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
a            | 1                                                                                                        
b            | 2                                                                                                        
c            | 3                                                                                                        
========================================================================================================================
```

Example 2. The `Stream` overload is equivalent — pass an already-built
`Stream` of labels directly.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
auto label_stream = pluss::kindle::Stream(pluss::make_note({"x", "y", "z"}));
auto labeled = stream.lbl().set(label_stream);
std::cout << labeled;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
x            | 1                                                                                                        
y            | 2                                                                                                        
z            | 3                                                                                                        
========================================================================================================================
```

## Notes

- See the important callout above.
- After `set()`, [`has()`](has.md) reports `true` — the label is now
  genuine, not a [`values()`](values.md) synthesized fallback.
- `labels` must match the bound stream's length; a mismatch throws.

## See Also

- [`StreamLabelAccessor::reset`](reset.md): the inverse operation, clearing
  the label back to unset.
- [`StreamLabelAccessor::has`](has.md): confirms a label was genuinely set.
- [`Stream::with_label`](../stream/with_label.md): a narrower set-only
  surface reached outside this accessor, equivalent in effect.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
