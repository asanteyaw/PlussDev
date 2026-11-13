<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3974092e3750 -->

# Stream::std

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::std(bool unbiased = true) const
```

## What It Does

Returns the standard deviation of a `Series`-kind stream's non-`NULL` values, as a
one-element `Stream` — the square root of [`var()`](var.md), with the identical
`unbiased` flag. Delegates to `Slate::std()`, so it's `Series`-only, same as
[`sum()`](sum.md)/[`mean()`](mean.md)/[`min()`](min.md)/[`max()`](max.md)/[`var()`](var.md).

## Parameters

**unbiased** (`bool`, defaults to `true`)
    `true` uses the Bessel-corrected sample variance (dividing by `n - 1`) before taking the
    square root; `false` uses the population variance (dividing by `n`).

## Returns

`Stream`
    A one-element `Series` stream holding the standard deviation.

## Examples

Example 1. Default (`unbiased=true`).

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.std();
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1.70782512766                                                                                           
========================================================================================================================
```

`1.70782512766` is `sqrt(2.91666666667)` — the square root of [`var()`](var.md)'s default
result for the same data.

Example 2. `unbiased=false` matches `sqrt(var(false))`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true, true, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}), mask);

std::cout << stream.std(false);
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1.47901994577                                                                                           
========================================================================================================================
```

## Notes

- `Series`-only; throws `"Slate::std is only valid for Series payload"` on
  `Note`/`Timeline`/`TimeIndex` streams.
- `NULL` rows are excluded, same as [`var()`](var.md)/[`mean()`](mean.md).
- Always equals `sqrt(var(unbiased))` for the same `unbiased` argument.

## See Also

- [`Stream::var`](var.md): the value this method takes the square root of.
- [`Stream::mean`](mean.md): the center this method measures dispersion around.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
