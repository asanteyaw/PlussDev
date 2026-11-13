<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b6eb0dde782 -->

# Stream::clear_label

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::clear_label() const
```

## What It Does

Returns a new stream with its row-label axis removed, leaving the receiver untouched —
the immutable counterpart to [`clear_label_()`](clear_label_.md). Calling it on a stream
that has no label already is a harmless no-op: it returns an equivalent unlabeled stream
rather than throwing.

## Parameters

`None`

## Returns

`Stream`
    A new stream, identical payload to the receiver, with no label axis.

## Examples

Example 1. Clear a label that was previously set.

```cpp
using pluss::table::Slate;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_label(Slate(pluss::make_note({"a", "b", "c"})));

auto cleared = labeled.clear_label();
std::cout << "labeled has_label: " << labeled.has_label() << "\n";
std::cout << "cleared has_label: " << cleared.has_label();
```

### Output

```text
labeled has_label: 1
cleared has_label: 0
```

Example 2. Calling it on an already-unlabeled stream is a harmless no-op.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.clear_label().has_label();
```

### Output

```text
0
```

## Notes

- Safe to call unconditionally, whether or not a label is currently set — never throws.
- Returns a new `Stream`; the receiver's own label (if any) is unaffected. Use
  [`clear_label_()`](clear_label_.md) to mutate in place instead.
- Equivalent in effect to [`lbl().reset()`](lbl.md), reached through the plain
  getter/setter surface rather than the `StreamLabelAccessor`.

## See Also

- [`Stream::clear_label_`](clear_label_.md): the mutating in-place version of this method.
- [`Stream::with_label`](with_label.md): the inverse operation — attach a label instead of
  removing one.
- [`Stream::lbl`](lbl.md)`.reset()`: the accessor-surface equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="clear_label_.html">
    <span class="label">Next</span>
    <span class="title">clear_label_</span>
  </a>
</div>
