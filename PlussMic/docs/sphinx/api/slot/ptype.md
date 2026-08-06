<!-- pluss-hand-authored -->

# Slot::ptype

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::string Slot::ptype() const
```

## What It Does

Returns the slot's value type as a string: the value type name shared by
every value (`"Series"`, `"Note"`, `"Timeline"`, `"TimeIndex"`), `"Mixed"`
if values differ in type, or `"Empty"` for a zero-value slot.

## Parameters

`None`

## Returns

`std::string`
    The shared value type name, `"Mixed"`, or `"Empty"`.

## Examples

Example 1. A homogeneous `Series` slot reports `"Series"`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)});
std::cout << slot.ptype();
```

### Output

```text
Series
```

Example 2. A homogeneous `Note` slot reports `"Note"`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("a"), pluss::make_note("bb")});
std::cout << slot.ptype();
```

### Output

```text
Note
```

Example 3. Mixing value types reports `"Mixed"`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x")});
std::cout << slot.ptype();
```

### Output

```text
Mixed
```

## Notes

- A string counterpart to [`kind()`](kind.md), which returns the
  `SlotKind` enum instead — `ptype()` is more convenient for logging or
  display, `kind()` for branching in code.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::kind`](kind.md): the enum form of this same classification.
- [`Slot::type_counts`](type_counts.md): a full per-value type tally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="push.html">
    <span class="label">Next</span>
    <span class="title">push</span>
  </a>
</div>
