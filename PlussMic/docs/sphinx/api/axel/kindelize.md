<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: abb09beec00e -->

# Axel::kindelize

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
pluss::kindle::Archive Axel::kindelize() const
```

## What It Does

Converts this `Axel` to its `kindle::Archive` representation. A
depth-stacked bridge from the bundle model into the Kindle model.

## Parameters

`None`

## Returns

`pluss::kindle::Archive`
    One archive catalog per registry, in axel depth order.

## Examples

Example 1. `kindelize()` preserves depth.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
});
auto archive = axel.kindelize();
std::cout << archive.depth();
```

### Output

```text
2
```

## Notes

- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Archive::bundelize`](../archive/bundelize.md): the reverse bridge from
  Kindle back into the bundle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="labels.html">
    <span class="label">Next</span>
    <span class="title">labels</span>
  </a>
</div>
