<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 43688efb6fa5 -->

# Stream::ffill

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::ffill() const
```

## What It Does

Returns a new stream with each mask-marked `NULL` cell replaced by the nearest preceding
non-`NULL` value ("forward fill") — the neighbor-based counterpart to
[`fillnull()`](fillnull.md)'s fixed-scalar fill. A `NULL` with no preceding value (e.g. at
row 0) has nothing to propagate from and stays `NULL`.

## Parameters

`None`

## Returns

`Stream`
    A new stream with mask-marked `NULL` cells filled from the preceding value where
    possible.

## Examples

Example 1. Forward-fill a gap in the middle of the stream.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.ffill();
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
1            | 1                                                                                                        
2            | 3                                                                                                        
3            | 3                                                                                                        
4            | 5                                                                                                        
========================================================================================================================
```

Example 2. A leading `NULL` has no preceding value to propagate from, so it stays `NULL`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{false, true, false});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

std::cout << stream.ffill();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | NA                                                                                                       
1            | 2                                                                                                        
2            | 2                                                                                                        
========================================================================================================================
```

## Notes

- A leading `NULL` (or a run of them at the very start) has nothing to fill from and
  remains `NULL` — see Example 2. Note the output's `mask=yes`, confirming a validity mask
  is still present on the result.
- Same mask-only scope as [`dropnull()`](dropnull.md) — an unmarked `NaN` in a numeric
  stream is not treated as fillable by `ffill()` any more than it's dropped by
  `dropnull()`.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::bfill`](bfill.md): the mirror operation, filling from the following value
  instead of the preceding one.
- [`Stream::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.
- [`Stream::dropnull`](dropnull.md): remove `NULL` rows instead of filling them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
