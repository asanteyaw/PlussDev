<!-- pluss-hand-authored -->

# Slot::invalid

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::invalid() const
```

## What It Does

Returns a new `Series[bool]` slot flagging each value that's missing *or*
non-finite — `std::monostate`, or a `NaN`/`Inf`/`-Inf` inside a
floating-point `Series` value. A broader check than [`isnull()`](isnull.md),
which only catches `NaN`, not `Inf`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot, same length as the source: `true` where the
    value is missing or non-finite, `false` otherwise.

## Examples

Example 1. Flag a null and a `NaN` among finite values.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1.0), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(std::nan(""))});
auto out = slot.invalid();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(2)).to_vec<bool>()[0];
```

### Output

```text
false true true
```

## Notes

- Broader than [`isnull()`](isnull.md): also flags `Inf`/`-Inf`, not just
  `NaN` and `std::monostate`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::isnull`](isnull.md): the narrower missing-only check.
- [`Slot::dropnull`](dropnull.md) / [`Slot::fillnull`](fillnull.md): act on
  missing values (not non-finite ones) after identifying them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
