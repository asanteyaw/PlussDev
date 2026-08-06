<!-- pluss-hand-authored -->

# Slot::kindelize

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
pluss::kindle::Stream Slot::kindelize() const
```

## What It Does

An identical alias for [`to_kindle()`](to_kindle.md), kept for
backward-compatible call sites written before `to_kindle()`/`as_kindle()`
were introduced.

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    The slot's values as a single kindle `Stream`.

## Examples

Example 1. Convert a homogeneous `Series` slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto stream = slot.kindelize();
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
- [`Slot::as_kindle`](as_kindle.md): another identical alias.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
