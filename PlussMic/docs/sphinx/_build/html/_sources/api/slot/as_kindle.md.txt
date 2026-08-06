<!-- pluss-hand-authored -->

# Slot::as_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
pluss::kindle::Stream Slot::as_kindle() const
```

## What It Does

An identical alias for [`to_kindle()`](to_kindle.md) — a semantic name for
call sites that read better as "view this as a kindle `Stream`" rather
than "convert this to a kindle `Stream`."

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    The slot's values as a single kindle `Stream`.

## Examples

Example 1. Convert a homogeneous `Series` slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto stream = slot.as_kindle();
std::cout << stream.as_series().numel();
```

### Output

```text
3
```

## Notes

- Byte-identical behavior to [`to_kindle()`](to_kindle.md), including its
  stream-compatibility requirement and throw text.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::to_kindle`](to_kindle.md): the primary name for this
  conversion, with the full stream-compatibility explanation.
- [`Slot::kindelize`](kindelize.md): another identical alias.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
