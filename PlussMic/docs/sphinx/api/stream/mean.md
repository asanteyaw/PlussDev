<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 26c16da5020b -->

# Stream::mean

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::mean() const
```

## What It Does

Returns the arithmetic mean of a `Series`-kind stream's non-`NULL` values, as a one-element
`Stream`. `NULL`-marked rows are excluded from both the sum and the divisor. Delegates to
`Slate::mean()`, so it's `Series`-only, same as [`sum()`](sum.md)/[`min()`](min.md)/
[`max()`](max.md)/[`var()`](var.md)/[`std()`](std.md).

## Parameters

`None`

## Returns

`Stream`
    A one-element `Series` stream holding the mean.

## Examples

Example 1. Mean of a stream with one masked `NULL` row.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.mean();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 3.25                                                                                                     
========================================================================================================================
```

`(1 + 3 + 4 + 5) / 4 = 3.25` — both the sum and the divisor exclude the masked row; the
divisor is `4` (non-`NULL` count), not `5` (total row count).

## Notes

- `Series`-only; throws `"Slate::mean is only valid for Series payload"` on
  `Note`/`Timeline`/`TimeIndex` streams.
- The divisor is the count of **non-`NULL`** rows, not the total row count — see Example 1.
- Returns a one-element `Stream`, not a raw scalar, same as every other reduction in this
  family.

## See Also

- [`Stream::sum`](sum.md): the numerator this method effectively divides.
- [`Stream::var`](var.md) / [`Stream::std`](std.md): dispersion around this mean.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="meta.html">
    <span class="label">Next</span>
    <span class="title">meta</span>
  </a>
</div>
