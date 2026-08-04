<!-- pluss-hand-authored -->

# Slot::size

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::int64_t Slot::size() const
```

## What It Does

Returns the number of values held in the slot.

## Parameters

`None`

## Returns

`std::int64_t`
    The count of values in the slot.

## Examples

Example 1. A three-value slot reports size `3`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
std::cout << slot.size();
```

### Output

```text
3
```

Example 2. An empty (but defined) slot reports size `0`.

```cpp
auto slot = pluss::bundle::Slot(std::vector<pluss::bundle::Slot::Value>{});
std::cout << slot.size();
```

### Output

```text
0
```

## Notes

- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.
- Counts every value, including `std::monostate` (null) entries.

## See Also

- [`Slot::empty`](empty.md): shorthand for `size() == 0`.
- [`Slot::shape`](shape.md): pairs `size()` with a fixed second element,
  matching `Registry::shape()`'s two-element convention.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
