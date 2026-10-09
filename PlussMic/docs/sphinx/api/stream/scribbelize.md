<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b44f43aadb0d -->

# Stream::scribbelize

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
pluss::scribble::Slot Stream::scribbelize() const
```

## What It Does

Converts the stream into a `pluss::scribble::Slot` — the reverse direction of
[`Slot::kindelize`](../slot/kindelize.md). A `Stream` is one typed, columnar payload
(`Series`, `Note`, `Timeline`, or `TimeIndex`) plus an optional per-element validity
mask; a `Slot` is a `std::vector<Slot::Value>`, where `Slot::Value` is
`std::variant<std::monostate, Series, Note, Timeline, TimeIndex>` — one independent,
individually-typed value per position. `scribbelize()` walks the stream element by
element, producing exactly `size()` `Slot::Value` entries, converting each present
element to a length-1 `Series`/`Note`/`Timeline`/`TimeIndex` scalar wrapper and each
null (masked-out) position to `std::monostate{}`.

## Parameters

`None`

## `StreamKind` conversion table

Which conversion path runs is decided once, up front, by the stream's own
[`kind()`](kind.md):

**`StreamKind::Series`**
    Dispatches further on the series' `scalar_type()`: `Bool` values are converted via
    an explicit length-1 `Bool` series per position; `Int64` values are converted via
    `pluss::make_series(value, scalar_type())`, preserving the exact integer scalar
    type; every other scalar type is read through `Float64` (`to(ScalarType::Float64)`)
    before being wrapped — so a `Float32` stream, for instance, is round-tripped through
    `double` precision on its way into the `Slot`.

**`StreamKind::Note`**
    Each position becomes a length-1 `Note` via `pluss::make_note(note.at(i))`.

**`StreamKind::Timeline`**
    Each position's epoch-nanosecond value is converted back to a `Timeline` via
    `pluss::timeline::from_epoch`, using the source timeline's own timezone.

**`StreamKind::TimeIndex`**
    Each position becomes a single-element `TimeIndex` (`pluss::timeindex::make`,
    non-sorted), built the same way as the `Timeline` case, from the index's own
    underlying timeline and timezone.

## Returns

`pluss::scribble::Slot`
    A slot with exactly `size()` values, one per stream position, in original order.

## Examples

Example 1. A `Series` stream (no nulls) — each position becomes an independent
length-1 `Series` scalar.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto slot = stream.scribbelize();
std::cout << slot.size() << "\n";
std::cout << std::get<pluss::Series>(slot.at(0)).numel() << "\n";
std::cout << std::get<pluss::Series>(slot.at(0)).to_vec<double>()[0] << "\n";
```

### Output

```text
3
1
10
```

Example 2. A `Note` stream — each position becomes an independent length-1 `Note`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"accra", "tema"}));
auto slot = stream.scribbelize();
std::cout << slot.size() << "\n";
std::cout << std::holds_alternative<pluss::Note>(slot.at(0)) << "\n";
```

### Output

```text
2
1
```

Example 3 (edge case, null preservation). A stream carrying a validity mask —
built here via `Slate`'s explicit mask constructor and bridged in through
[`Stream::from_slate`](from_slate.md) — converts masked-out positions to
`std::monostate{}` rather than any placeholder `Series` value.

```cpp
auto raw_series = pluss::make_series({1.0, 2.0, 3.0});
std::vector<bool> mask_values{true, false, true};
auto mask_series = pluss::make_series(mask_values);
pluss::table::Slate slate_with_nulls(raw_series, mask_series);
auto stream = pluss::kindle::Stream::from_slate(slate_with_nulls);
auto slot = stream.scribbelize();
std::cout << std::holds_alternative<std::monostate>(slot.at(0)) << "\n";
std::cout << std::holds_alternative<std::monostate>(slot.at(1)) << "\n";
std::cout << std::holds_alternative<std::monostate>(slot.at(2)) << "\n";
```

### Output

```text
0
1
0
```

Example 4 (edge case). An undefined stream throws before any position is visited.

```cpp
try {
  pluss::kindle::Stream undefined_stream;
  undefined_stream.scribbelize();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Stream::scribbelize: stream undefined
```

## Notes

- Requires `defined()` — an undefined stream throws
  `"Stream::scribbelize: stream undefined"` immediately.
- Any stream label ([`has_label`](has_label.md)) is **not** carried onto the resulting
  `Slot` — `Slot` has no equivalent per-element label concept, so label information is
  dropped by this conversion. If you need the label preserved, read it separately via
  [`Stream::label`](label.md) before calling `scribbelize()`.
- A round trip through `Slot::kindelize()` afterward is not guaranteed to reproduce a
  stream that is `defined()`-identical to the original in every internal respect
  (e.g. a mixed-`Slot` built independently of this method may reject `kindelize()`
  outright if its values aren't kind-compatible) — but a `Slot` produced by
  `scribbelize()` itself is always internally kind-consistent, since every value came
  from the same source stream's single `StreamKind`.
- `Bool`-typed `Series` streams are explicitly special-cased ahead of the general
  numeric path — see the conversion table above — since `to_vec<bool>()`/`Bool` series
  construction need their own code path distinct from the other scalar types.

## See Also

- [`Slot::kindelize`](../slot/kindelize.md): the reverse bridge, from the scribble model
  back into Kindle.
- [`Catalog::scribbelize`](../catalog/scribbelize.md): the catalog-level sibling that calls
  this method once per key.
- [`Stream::kind`](kind.md): what determines which conversion path runs.
- [`Stream::tabelize`](tabelize.md) / [`Stream::to_slate`](to_slate.md): the sibling
  bridge from Kindle into the table model, instead of the scribble model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
