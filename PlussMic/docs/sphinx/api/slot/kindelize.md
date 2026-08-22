<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb77e2c3fe47 -->

# Slot::kindelize

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
pluss::kindle::Stream Slot::kindelize() const
```

## What It Does

Converts the slot into a single `pluss::kindle::Stream`. This is the canonical
scribble-to-kindle bridge for one heterogeneous-but-kind-compatible slot.

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    The slot's values as a single kindle `Stream`.

## Examples

Example 1. Convert a homogeneous `Series` slot.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto stream = slot.kindelize();
std::cout << stream.as_series().numel();
```

### Output

```text
3
```

## Notes

- Every value in the slot must share one stream-compatible kind.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.
- A slot that later goes through `stream.tabelize()` or `stream.scribbelize()` stays on the
  same 1D bridge family; no table or registry wrapper is required.

## See Also

- [`Stream::scribbelize`](../stream/scribbelize.md): the reverse bridge from
  Kindle back into the scribble model.
- [`Slot::tabelize`](tabelize.md): the sibling bridge from the scribble model into the
  table model.

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
