<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 964c1ee22ae6 -->

# Stream::clear_label_

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream& Stream::clear_label_()
```

## What It Does

The mutating, in-place counterpart to [`clear_label()`](clear_label.md): removes the row-
label axis from the receiver directly and returns a reference to `*this`, rather than
allocating a new `Stream`. Like the immutable version, calling it when no label is set is a
harmless no-op.

## Parameters

`None`

## Returns

`Stream&`
    A reference to `*this`, now with no label axis — enabling chained calls on the same
    object.

## Examples

Example 1. Mutate a labeled stream in place and confirm it's the same object.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_label(Slate(pluss::make_note({"a", "b", "c"})));

auto& ref = stream.clear_label_();
std::cout << "same object: " << (&ref == &stream) << "\n";
std::cout << "has_label after clear_label_: " << stream.has_label();
```

### Output

```text
same object: 1
has_label after clear_label_: 0
```

## Notes

- Returns `*this` by reference, unlike `clear_label()`'s fresh `Stream` — mutates the
  receiver directly.
- Never throws, whether or not a label was set beforehand.
- Requires a non-`const` `Stream` to call, since it mutates the receiver.

## See Also

- [`Stream::clear_label`](clear_label.md): the immutable version, returning a new `Stream`
  instead of mutating in place.
- [`Stream::with_label_`](with_label_.md): the mutating counterpart for attaching a label.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_label.html">
    <span class="label">Previous</span>
    <span class="title">clear_label</span>
  </a>
  <a class="next-link" href="clip.html">
    <span class="label">Next</span>
    <span class="title">clip</span>
  </a>
</div>
