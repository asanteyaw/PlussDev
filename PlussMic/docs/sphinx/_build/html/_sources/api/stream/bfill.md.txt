<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5f0a62b8b782 -->

# Stream::bfill

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::bfill() const -> Stream
```

## What It Does

Returns a new stream with each mask-marked `NULL` cell replaced by the nearest following
non-`NULL` value ("backward fill") — the mirror of [`ffill()`](ffill.md). A `NULL` with no
following value (e.g. at the last row) has nothing to propagate from and stays `NULL`.

## Parameters

`None`

## Returns

`Stream`
    A new stream with mask-marked `NULL` cells filled from the following value where
    possible.

## Examples

Example 1. Backward-fill a gap in the middle of the stream.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.bfill();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=5
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 3                                                                                                        
2            | 3                                                                                                        
3            | 5                                                                                                        
4            | 5                                                                                                        
========================================================================================================================
```

Example 2. A trailing `NULL` has no following value to propagate from, so it stays `NULL`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

std::cout << stream.bfill();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2                                                                                                        
1            | 2                                                                                                        
2            | NA                                                                                                       
========================================================================================================================
```

## Notes

- A trailing `NULL` (or a run of them at the very end) has nothing to fill from and remains
  `NULL` — see Example 2.
- Same mask-only scope as [`dropnull()`](dropnull.md)/[`ffill()`](ffill.md) — an unmarked
  `NaN` is not treated as fillable.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::ffill`](ffill.md): the mirror operation, filling from the preceding value
  instead of the following one.
- [`Stream::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.
- [`Stream::dropnull`](dropnull.md): remove `NULL` rows instead of filling them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
