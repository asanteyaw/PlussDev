<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ac2393a4917 -->

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
auto slot = pluss::scribble::Slot({pluss::make_series(1.0), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(std::nan(""))});
auto out = slot.invalid();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(2)).host<bool>()[0];
```

### Output

```text
false true true
```

## Notes

- `invalid` is the union of the three kinds: `true` for `NULL` ([`isnull`](isnull.md)), `NaN` ([`isnan`](isnan.md)) and both infinities ([`isinf`](isinf.md)). Its partners are [`dropinvalid`](dropinvalid.md) and [`fillinvalid`](fillinvalid.md). See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
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
  <a class="next-link" href="isinf.html">
    <span class="label">Next</span>
    <span class="title">isinf</span>
  </a>
</div>
