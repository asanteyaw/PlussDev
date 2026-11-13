<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5b88e4856d62 -->

# Stream::with_label

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::with_label(const pluss::table::Slate& label) const
```

## What It Does

Returns a **new** stream carrying `label` as its row-label axis, leaving the receiver
untouched — the immutable counterpart to [`with_label_()`](with_label_.md). `label` must be
the same length as the stream's own payload; a mismatch throws before any copy is made.

```{important} label length must match the stream's own length
Unlike alignment operations (which tolerate length differences by design), `with_label()`
requires `label.size() == this->size()` exactly. A mismatch throws `"Stream: label axis
length must match payload length"` — check lengths up front if `label` comes from a
different source than the stream's own row count.
```

## Parameters

**label** (`const pluss::table::Slate&`)
    The row-label axis to attach. Commonly a `Note`-kind slate of string labels, but any
    `Slate` payload kind is accepted. Must match the stream's own length.

## Returns

`Stream`
    A new stream, identical payload to the receiver, with `label` attached as its label
    axis.

## Examples

Example 1. Attach a `Note`-kind label to a numeric stream.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto labels = Slate(pluss::make_note({"a", "b", "c"}));

auto out = stream.with_label(labels);
std::cout << "original has_label: " << stream.has_label() << "\n";
std::cout << "out has_label: " << out.has_label() << "\n";
std::cout << out.label().value();
```

### Output

```text
original has_label: 0
out has_label: 1
0  a
1  b
2  c
ptype: Note
```

Example 2. A length mismatch throws immediately, before the receiver is touched.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto too_short = Slate(pluss::make_note({"a", "b"}));

try {
  auto out = stream.with_label(too_short);
} catch (const std::exception& e) {
  std::cout << "threw (message starts): "
            << std::string_view(e.what()).substr(0, 51);
}
```

### Output

```text
threw (message starts): Stream: label axis length must match payload length
```

## Notes

- Requires an exact length match with the stream's payload — see the important callout
  above. This is stricter than `lbl().align()`, which is built specifically to reconcile
  length/order mismatches instead of rejecting them.
- Returns a new `Stream`; the receiver is unmodified. Use [`with_label_()`](with_label_.md)
  to mutate in place instead.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above (this build enables it for this internal validation path) — examples here
  show only the leading message for readability.

## See Also

- [`Stream::with_label_`](with_label_.md): the mutating in-place version of this method.
- [`Stream::clear_label`](clear_label.md): remove a label axis instead of attaching one.
- [`Stream::lbl`](lbl.md)`.set()`: the accessor-surface equivalent, accepting either a
  `Stream` or a `Slate`.
- [`Stream::lbl`](lbl.md)`.align()`: reconciles length/order mismatches instead of
  rejecting them outright.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="with_label_.html">
    <span class="label">Next</span>
    <span class="title">with_label_</span>
  </a>
</div>
