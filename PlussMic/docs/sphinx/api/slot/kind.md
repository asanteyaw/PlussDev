<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fba42e1dd254 -->

# Slot::kind

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
SlotKind Slot::kind() const
```

## What It Does

Classifies the slot's values into one of seven `SlotKind` categories:

```cpp
enum class SlotKind : std::uint8_t {
  Empty = 0,   // zero values
  Null,        // every value is std::monostate (null)
  Series,      // every value is Series, all the same scalar dtype
  Note,        // every value is Note
  Timeline,    // every value is Timeline
  TimeIndex,   // every value is TimeIndex
  Mixed        // anything else (mixed kinds, or Series with differing dtypes)
};
```

## Parameters

`None`

## Returns

`SlotKind`
    The slot's classification.

## Examples

Example 1. A homogeneous `int64` `Series` slot classifies as `Series`.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)});
std::cout << (slot.kind() == pluss::scribble::SlotKind::Series);
```

### Output

```text
1
```

Example 2. A slot mixing `Series` and `Note` values classifies as `Mixed` —
so does a `Series` slot whose values don't all share one scalar dtype.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_note("x")});
std::cout << (slot.kind() == pluss::scribble::SlotKind::Mixed);
```

### Output

```text
1
```

## Notes

- `Empty` (zero values) and `Null` (every value present but `std::monostate`)
  are distinct kinds — an empty slot and an all-null slot are not the same
  thing.
- A `Series` slot only stays `Series`-kind if every value shares the exact
  same scalar dtype; mixing `int64` and `float64` values demotes the slot to
  `Mixed`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::ptype`](ptype.md): the same classification rendered as a string,
  plus finer type-name detail.
- [`Slot::type_counts`](type_counts.md): a full per-value type tally, useful
  for diagnosing exactly *why* a slot came out `Mixed`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
