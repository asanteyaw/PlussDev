<!-- pluss-hand-authored -->

# Axel::registries

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
const std::vector<Registry>& Axel::registries() const
```

## What It Does

Returns a const reference to the full vector of `Registry` frames
backing this `Axel`, in depth order.

## Parameters

`None`

## Returns

`const std::vector<Registry>&`
    Every registry frame, index `0` through `depth() - 1`.

## Examples

Example 1. Read the frame count and the first frame's field list
straight off the returned vector.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
});
const auto& regs = axel.registries();
std::cout << "count=" << regs.size() << " nfields0=" << regs[0].nfields();
```

### Output

```text
count=2 nfields0=1
```

## Notes

- Returns a reference into the `Axel`'s own storage — no copy of the
  vector or its registries happens until you copy an individual
  `Registry` out of it (registries are themselves cheap, reference-counted
  handles, same as `Axel` and `Slot`).
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::registry`](registry.md): fetch one frame by index, with
  bounds checking and a clear out-of-range throw.
- [`Axel::depth`](depth.md): the vector's size.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="registry.html">
    <span class="label">Next</span>
    <span class="title">registry</span>
  </a>
</div>
