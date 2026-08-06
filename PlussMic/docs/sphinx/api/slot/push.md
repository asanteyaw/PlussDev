<!-- pluss-hand-authored -->

# Slot::push

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::push(const Value& value) const
```

## What It Does

Returns a new slot with `value` appended as the last entry. The source
slot is left unchanged.

## Parameters

**value** (`const Slot::Value&`)
    The value to append — any of `std::monostate`, `pluss::Series`,
    `pluss::Note`, `pluss::Timeline`, or `pluss::TimeIndex`.

## Returns

`Slot`
    A new slot, one value longer than the source.

## Examples

Example 1. Append a fourth value.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.push(pluss::make_series(4));
std::cout << "orig=" << slot.size() << " new=" << out.size();
```

### Output

```text
orig=3 new=4
```

## Notes

- Non-mutating — returns a new `Slot`; the source is untouched. See
  [`push_()`](push_.md) for the in-place equivalent.
- Appending a value of a different kind than the rest of the slot demotes
  the resulting slot's [`kind()`](kind.md) to `Mixed`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::push_`](push_.md): the in-place variant.
- [`Slot::concat`](concat.md): append every value from another `Slot` at
  once, rather than one value at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ptype.html">
    <span class="label">Previous</span>
    <span class="title">ptype</span>
  </a>
  <a class="next-link" href="push_.html">
    <span class="label">Next</span>
    <span class="title">push_</span>
  </a>
</div>
