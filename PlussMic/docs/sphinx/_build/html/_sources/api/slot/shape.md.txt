<!-- pluss-hand-authored -->

# Slot::shape

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::pair<std::int64_t, std::int64_t> Slot::shape() const
```

## What It Does

Returns `{size(), 1}` — the slot's value count paired with a fixed `1`.

```{important} The second element is always 1
A `Slot` is an inherently one-dimensional sequence of values, so the second
element of `shape()` never varies — it exists purely so `Slot::shape()`
matches the two-element convention used by [`Registry::shape()`](../registry/shape.md)
(`{nfields, size}`), rather than conveying real structure of its own.
```

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `{size(), 1}`.

## Examples

Example 1. A three-value slot reports `(3, 1)`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto sh = slot.shape();
std::cout << "(" << sh.first << ", " << sh.second << ")";
```

### Output

```text
(3, 1)
```

## Notes

- See the important callout above — the second element is a fixed `1`,
  not meaningful shape information.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"` (via `size()`).

## See Also

- [`Slot::size`](size.md): the value actually carried by `shape()`'s first
  element.
- [`Registry::shape`](../registry/shape.md): the two-element convention
  `shape()` mirrors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="push_.html">
    <span class="label">Previous</span>
    <span class="title">push_</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
