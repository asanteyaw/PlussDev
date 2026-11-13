<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 05ce8efb26fb -->

# Stream::with_label_

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream& Stream::with_label_(const pluss::table::Slate& label)
```

## What It Does

The mutating, in-place counterpart to [`with_label()`](with_label.md): attaches `label` as
the stream's row-label axis and returns a reference to `*this`, rather than allocating a
new `Stream`. Same length requirement as the immutable overload — `label` must match the
stream's own length exactly, or it throws before any state changes.

## Parameters

**label** (`const pluss::table::Slate&`)
    The row-label axis to attach in place. Must match the stream's own length.

## Returns

`Stream&`
    A reference to `*this`, now carrying `label` as its label axis — enabling chained calls
    on the same object.

## Examples

Example 1. Mutate a stream in place and confirm it's the same object.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto labels = Slate(pluss::make_note({"x", "y", "z"}));

auto& ref = stream.with_label_(labels);
std::cout << "same object: " << (&ref == &stream) << "\n";
std::cout << "stream has_label: " << stream.has_label() << "\n";
```

### Output

```text
same object: 1
stream has_label: 1
```

Example 2. A length mismatch throws and leaves the stream's own label unmodified.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto too_long = Slate(pluss::make_note({"a", "b", "c", "d"}));

try {
  stream.with_label_(too_long);
} catch (const std::exception&) {
  std::cout << "threw; has_label still: " << stream.has_label();
}
```

### Output

```text
threw; has_label still: 0
```

## Notes

- Returns `*this` by reference, unlike `with_label()`'s fresh `Stream` — mutates the
  receiver directly.
- Same length-match requirement, and same throw message, as
  [`with_label()`](with_label.md); see that page's important callout.
- Requires a non-`const` `Stream` to call, since it mutates the receiver.

## See Also

- [`Stream::with_label`](with_label.md): the immutable version, returning a new `Stream`
  instead of mutating in place.
- [`Stream::clear_label_`](clear_label_.md): the mutating counterpart for removing a label.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_label.html">
    <span class="label">Previous</span>
    <span class="title">with_label</span>
  </a>
  <a class="next-link" href="with_meta.html">
    <span class="label">Next</span>
    <span class="title">with_meta</span>
  </a>
</div>
