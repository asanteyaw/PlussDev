<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5a02e4795752 -->

# Slot::concat

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::concat(const Slot& other) const
```

## What It Does

Returns a new slot with every value from `other` appended after every
value from the source slot.

## Parameters

**other** (`const Slot&`)
    The slot whose values are appended. Must be defined.

## Returns

`Slot`
    A new slot, `size() + other.size()` values long.

## Examples

Example 1. Concatenate two slots.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto other = pluss::scribble::Slot({pluss::make_series(4), pluss::make_series(5)});
auto out = slot.concat(other);
std::cout << "size=" << out.size() << " last_two="
          << std::get<pluss::Series>(out.at(3)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(4)).host<std::int64_t>()[0];
```

### Output

```text
size=5 last_two=4,5
```

## Notes

- `other` must be defined — an undefined `other` throws
  `"Slot::concat: other slot is undefined"`.
- Concatenating slots of different kinds produces a `Mixed`-kind result if
  the combined values no longer share one kind — see [`kind()`](kind.md).
- Requires the source slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::push`](push.md): append one value at a time instead of every
  value from another slot at once.
- [`Registry::concat`](../registry/concat.md): the field-scoped equivalent
  at the `Registry` level.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
