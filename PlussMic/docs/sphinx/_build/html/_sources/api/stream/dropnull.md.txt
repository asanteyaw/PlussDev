<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d64077ca9882 -->

# Stream::dropnull

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::dropnull() const -> Stream
```

## What It Does

Returns a new stream with every row that has an explicit **validity-mask** `NULL` removed,
preserving order for the remaining rows.

```{important} dropnull() only removes mask-marked NULLs — not unmarked NaN
`dropnull()` does **not** agree with [`isnull()`](isnull.md)/[`invalid()`](invalid.md) in
every case: those two report an unmarked `NaN` in a numeric stream as invalid (`true`), but
`dropnull()` leaves that row in place — it only removes rows carrying an explicit validity
mask. If you need to drop unmarked `NaN` rows too, filter explicitly with
`stream.filter(stream.notnull().as_series())` instead of relying on `dropnull()` alone.
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
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 30                                                                                                       
========================================================================================================================
```

Example 2. An unmarked `NaN` (no validity mask at all) is *not* removed by `dropnull()`,
even though [`isnull()`](isnull.md) reports it as `true` — see the important callout above.

```cpp
double nan_value = std::numeric_limits<double>::quiet_NaN();
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, nan_value, 30.0}));

std::cout << "isnull reports it invalid: " << stream.isnull();
std::cout << "dropnull size (unchanged): " << stream.dropnull().size();
```

### Output

```text
isnull reports it invalid: ========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
2            | false                                                                                                    
========================================================================================================================
dropnull size (unchanged): 3
```

## Notes

- Only removes mask-marked `NULL` rows — see the important callout above for the
  discrepancy with `isnull()`/`invalid()` on unmarked `NaN`.
- Preserves the relative order of the remaining rows.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::isnull`](isnull.md) / [`Stream::invalid`](invalid.md): the broader validity
  checks this method does *not* fully agree with — see the important callout above.
- [`Stream::filter`](filter.md): the general-purpose tool to reproduce a stricter drop
  (including unmarked `NaN`) via `stream.filter(stream.notnull().as_series())`.
- [`Stream::fillnull`](fillnull.md) / [`Stream::ffill`](ffill.md) / [`Stream::bfill`](bfill.md):
  fill mask-marked `NULL`s instead of removing their rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_meta_.html">
    <span class="label">Previous</span>
    <span class="title">drop_meta_</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
