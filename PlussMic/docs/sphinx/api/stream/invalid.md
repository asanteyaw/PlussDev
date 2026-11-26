<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b0ebabe5fe33 -->

# Stream::invalid

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::invalid() const
```

## What It Does

Returns a new `Bool`-dtype `Series` stream, same length as the receiver, marking which records
are not ordinary numbers: `true` for a missing record (`NULL`), a `NaN` or an infinity. It is the
union of [`isnull()`](isnull.md), [`isnan()`](isnan.md) and [`isinf()`](isinf.md), so it agrees with
`isnull()` for a missing record and also flags the other two, which `isnull()` does not (a `NaN`
is a value, not a missing record). Delegates to `Shelf::invalid()`, and works on all four payload
kinds (text, timestamp and duration streams can only be missing, so there `invalid()` equals
`isnull()`).

## Parameters

`None`

## Returns

`Stream`
    A new `Series`-kind stream of `bool` values: `true` where the row is invalid, `false`
    otherwise.

## Examples

Example 1. `invalid()` reports the same rows as `isnull()` for a mask-based `NULL`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0}), mask);

std::cout << stream.invalid();
```

### Output

```text
0  false
1  true
2  false
3  true
ptype: Series
```

Example 2. A `NaN` is invalid but not missing: `invalid()` flags it and `isnull()` does not.

```cpp
double nan_value = std::numeric_limits<double>::quiet_NaN();
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, nan_value, 3.0}));

std::cout << stream.invalid();
std::cout << stream.isnull();
```

### Output

```text
0  false
1  true
2  false
ptype: Series
0  false
1  false
2  false
ptype: Series
```

## Notes

- `invalid` is the union of the three kinds: `true` for `NULL` ([`isnull`](isnull.md)), `NaN` ([`isnan`](isnan.md)) and both infinities ([`isinf`](isinf.md)). Its partners are [`dropinvalid`](dropinvalid.md) and [`fillinvalid`](fillinvalid.md). See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Works on every payload kind — no `Series`-only restriction.
- `invalid()` and [`isnull()`](isnull.md) agree on missing records and differ on `NaN` and
  infinities: those are values, so only `invalid()` flags them.
- The result is always `Series`-kind (`Series[bool]`), regardless of the receiver's own
  kind.

## See Also

- [`Stream::isnull`](isnull.md) / [`Stream::notnull`](notnull.md): the missing-record
  checks; [`Stream::isnan`](isnan.md) and [`Stream::isinf`](isinf.md) cover the other two kinds.
- [`Stream::fillnull`](fillnull.md) / [`Stream::dropnull`](dropnull.md): act on the rows
  this method identifies.

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
