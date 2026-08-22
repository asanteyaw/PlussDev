<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 357571f594b5 -->

# Slot::notnull

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::notnull() const
```

## What It Does

Returns a new `Series[bool]` slot flagging each value that's *not*
missing. The exact logical inverse of [`isnull()`](isnull.md).

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot, same length as the source: `true` where the
    value is present, `false` where it's missing.

## Examples

Example 1. Flag which values are present.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)});
auto out = slot.notnull();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(2)).to_vec<bool>()[0];
```

### Output

```text
true false true
```

## Notes

- Exactly `!isnull()`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::isnull`](isnull.md): the logical inverse.
- [`Slot::dropnull`](dropnull.md): keep only the values `notnull()` would
  flag `true`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="nt.html">
    <span class="label">Next</span>
    <span class="title">nt</span>
  </a>
</div>
