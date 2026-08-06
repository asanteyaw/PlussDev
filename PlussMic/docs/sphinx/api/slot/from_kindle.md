<!-- pluss-hand-authored -->

# Slot::from_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
static Slot Slot::from_kindle(const pluss::kindle::Stream& stream)
```

## What It Does

Converts a kindle `Stream` into a `Slot`, unpacking the stream's single
shared column into independent per-value entries — the inverse of
[`to_kindle()`](to_kindle.md).

## Parameters

**stream** (`const pluss::kindle::Stream&`)
    The stream to convert. Must be defined.

## Returns

`Slot`
    A new slot, one value per row in `stream`.

## Examples

Example 1. Round-trip a `Series` stream through `Slot`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
auto slot = pluss::bundle::Slot::from_kindle(stream);
std::cout << "size=" << slot.size() << " val0="
          << std::get<pluss::Series>(slot.at(0)).to_vec<double>()[0];
```

### Output

```text
size=3 val0=10
```

## Notes

- `stream` must be defined — an undefined stream throws
  `"Slot::from_kindle: stream undefined"`.
- A row with a null validity flag becomes `std::monostate` in the
  resulting slot.
- A static method — call it as `Slot::from_kindle(stream)`, not on an
  existing instance.

## See Also

- [`Slot::to_kindle`](to_kindle.md) / [`Slot::as_kindle`](as_kindle.md) /
  [`Slot::kindelize`](kindelize.md): the inverse conversion.
- [`Registry::from_kindle`](../registry/from_kindle.md): the equivalent
  conversion at the `Registry` level, from a `Catalog`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
