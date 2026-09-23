<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a0012daf9957 -->

# Slot::isnull

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::isnull() const
```

## What It Does

Returns a new `Series[bool]` slot flagging each value that's missing —
`std::monostate`, or a `NaN` inside a floating-point `Series` value.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot, same length as the source: `true` where the
    value is missing, `false` otherwise.

## Examples

Example 1. Flag a null in the middle of a slot.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)});
auto out = slot.isnull();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(2)).to_vec<bool>()[0];
```

### Output

```text
false true false
```

## Notes

- Considers both `std::monostate` and `NaN` (inside `Series[float32]`/
  `Series[float64]` values) as missing — see [`invalid()`](invalid.md) for
  the related, broader check.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::notnull`](notnull.md): the exact logical inverse.
- [`Slot::invalid`](invalid.md): a broader check that also flags non-finite
  values like `Inf`.
- [`Slot::dropnull`](dropnull.md) / [`Slot::fillnull`](fillnull.md): act on
  the missing values instead of just flagging them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="kind.html">
    <span class="label">Next</span>
    <span class="title">kind</span>
  </a>
</div>
