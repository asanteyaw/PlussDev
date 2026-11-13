<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d954dc84a979 -->

# Stream::var

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::var(bool unbiased = true) const
```

## What It Does

Returns the variance of a `Series`-kind stream's non-`NULL` values, as a one-element
`Stream`. `unbiased` (default `true`) selects Bessel's correction — dividing the sum of
squared deviations by `n - 1` instead of `n`. Delegates to `Slate::var()`, so it's
`Series`-only, same as [`sum()`](sum.md)/[`mean()`](mean.md)/[`min()`](min.md)/
[`max()`](max.md)/[`std()`](std.md).

## Parameters

**unbiased** (`bool`, defaults to `true`)
    `true` divides by `n - 1` (sample variance, Bessel-corrected); `false` divides by `n`
    (population variance).

## Returns

`Stream`
    A one-element `Series` stream holding the variance.

## Examples

Example 1. Default (`unbiased=true`) divides by `n - 1`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.var();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2.91666666667                                                                                           
========================================================================================================================
```

Example 2. `unbiased=false` divides by `n` instead, giving a smaller value.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.var(false);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2.1875                                                                                                   
========================================================================================================================
```

## Notes

- `Series`-only; throws `"Slate::var is only valid for Series payload"` on
  `Note`/`Timeline`/`TimeIndex` streams.
- `NULL` rows are excluded from both the mean and the divisor `n`, same as
  [`mean()`](mean.md).
- Defaults to the Bessel-corrected sample variance (`unbiased=true`); pass `false`
  explicitly for the population variance.

## See Also

- [`Stream::std`](std.md): the square root of this method's result, in the same units as
  the original data.
- [`Stream::mean`](mean.md): the center this method measures dispersion around.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
  <a class="next-link" href="with_label.html">
    <span class="label">Next</span>
    <span class="title">with_label</span>
  </a>
</div>
