<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d64077ca9882 -->

# Stream::dropnull

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::dropnull() const
```

## What It Does

Returns a new stream with every row that has an explicit **validity-mask** `NULL` removed,
preserving order for the remaining rows.

```{important} dropnull() removes missing records only — a NaN is a value and stays
`dropnull()` removes exactly the records [`isnull()`](isnull.md) flags: the ones the validity
mask marks missing. A `NaN` or an infinity is a value, so it stays. Use
[`dropnan()`](dropnan.md), [`dropinf()`](dropinf.md) or [`dropinvalid()`](dropinvalid.md) to
remove those.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with mask-marked `NULL` rows removed.

## Examples

Example 1. A stream with an explicit validity mask: `dropnull()` removes the masked row.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}), mask);

std::cout << stream.dropnull();
```

### Output

```text
0  10
1  30
ptype: Series
```

Example 2. A `NaN` is *not* removed by `dropnull()`, because it is a value and not a missing
record; [`isnull()`](isnull.md) is `false` for it too.

```cpp
const double nan = std::numeric_limits<double>::quiet_NaN();
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, nan, 30.0}));

std::cout << "isnull is false for the NaN: " << stream.isnull();
std::cout << "dropnull size (unchanged): " << stream.dropnull().size();
```

### Output

```text
isnull is false for the NaN: 0  false
1  false
2  false
ptype: Series
dropnull size (unchanged): 3
```

## Notes

- `dropnull` removes missing cells only; a `NaN` or an infinity is a value and stays. Use [`dropnan`](dropnan.md), [`dropinf`](dropinf.md) or [`dropinvalid`](dropinvalid.md) for those. See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Only removes records that [`isnull()`](isnull.md) flags; it agrees with `isnull()` in every
  case (a `NaN` is neither flagged nor removed).
- Preserves the relative order of the remaining rows.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::isnull`](isnull.md): flags exactly the records this method removes;
  [`Stream::invalid`](invalid.md) is the broader check that also flags `NaN` and infinities.
- [`Stream::dropinvalid`](dropinvalid.md): remove missing records, `NaN` and infinities together.
- [`Stream::fillnull`](fillnull.md) / [`Stream::ffill`](ffill.md) / [`Stream::bfill`](bfill.md):
  fill mask-marked `NULL`s instead of removing their rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropneginf.html">
    <span class="label">Previous</span>
    <span class="title">dropneginf</span>
  </a>
  <a class="next-link" href="dropposinf.html">
    <span class="label">Next</span>
    <span class="title">dropposinf</span>
  </a>
</div>
