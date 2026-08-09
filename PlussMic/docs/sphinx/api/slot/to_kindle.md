<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb98600450b2 -->

# Slot::to_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
pluss::kindle::Stream Slot::to_kindle() const
```

## What It Does

Converts the slot to a `pluss::kindle::Stream` — the bridge from the
bundle model's per-value storage to the kindle model's single-column
storage. This is what every accessor (`nt()`, `tl()`, `cast()`,
`distinct()`, and more) routes through internally.

```{important} Requires the slot to be stream-compatible
`to_kindle()` throws unless every value in the slot shares one kind and,
for `Series` values, one scalar dtype — i.e. the slot's [`kind()`](kind.md)
must not be `Mixed`. The one exception is `Empty` (zero values), which
converts successfully to an empty `Note`-kind stream rather than throwing.
```

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    The slot's values as a single kindle `Stream`.

## Examples

Example 1. Convert a homogeneous `Series` slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto stream = slot.to_kindle();
std::cout << stream.as_series().numel();
```

### Output

```text
3
```

Example 2. A `Mixed`-kind slot throws — see the important callout above.

```cpp
auto mixed = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x")});
try {
  auto stream = mixed.to_kindle();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::to_kindle: slot is not stream-compatible (kind=Mixed)
```

## Notes

- See the important callout above — this is the check nearly every other
  `Slot` method (`nt()`, `tl()`, `cast()`, `distinct()`, `sum()`, and more)
  relies on internally.
- `Empty` slots (zero values) are the one exception — they convert to an
  empty `Note`-kind stream rather than throwing.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::as_kindle`](as_kindle.md) / [`Slot::kindelize`](kindelize.md):
  identical aliases for this same conversion.
- [`Slot::from_kindle`](from_kindle.md): the inverse conversion.
- [`Slot::kind`](kind.md): check whether a slot is stream-compatible
  before calling `to_kindle()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
