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
bundle-to-kindle bridge for one heterogeneous-but-kind-compatible slot.

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

- Every value in the slot must share one stream-compatible kind.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.
- A slot that later goes through `stream.tabelize()` or `stream.bundelize()` stays on the
  same 1D bridge family; no table or registry wrapper is required.

## See Also

- [`Stream::bundelize`](../stream/bundelize.md): the reverse bridge from
  Kindle back into the bundle model.
- [`Slot::tabelize`](tabelize.md): the sibling bridge from the bundle model into the
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
