<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb06bb07f691 -->

# Slate::median

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::median() const -> SlateValue
```

## What It Does

Returns the median of every observed (non-missing) value in a numeric `Series`
slate, as a scalar `SlateValue` — the middle value when the observed values
are sorted, less sensitive to outliers than [`mean()`](mean.md). For an
even number of observed values, it's the average of the two middle values
(the same linear-interpolation convention [`describe()`'s](describe.md) `50%`
quartile uses). Missing entries are excluded first, exactly like `mean()`. The
computation is carried out in `Float64` (or `Float32` off-CPU), regardless of
the source dtype. Restricted to `Series` payloads; every other kind throws. An
empty slate, or one where every entry is missing, also throws.

See [`Slate::sum`'s note on printing a `SlateValue`](sum.md) — every example
below uses `.to_dbl()` to extract a clean number instead of streaming the
`SlateValue` directly.

## Parameters

`None`

## Returns

`SlateValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the median of the
    observed values.

## Examples

Example 1. Median of an odd-length slate — the exact middle value.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({1.0, 9.0, 5.0}));
auto out = scores.median();
std::cout << out.to_dbl();
```

### Output

```text
5
```

Example 2. Median of an even-length slate — the average of the two middle
values.

```cpp
auto scores2 = Slate(pluss::make_series({1.0, 3.0, 5.0, 9.0}));
auto out2 = scores2.median();
std::cout << out2.to_dbl();
```

### Output

```text
4
```

The two middle values (sorted) are `3` and `5`; the median is their average,
`4` — not either value alone.

Example 3. Missing entries are excluded before finding the median.

```cpp
auto with_gap = Slate(
    pluss::make_series({1.0, 100.0, 5.0, 9.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out3 = with_gap.median();
std::cout << out3.to_dbl();
```

### Output

```text
5
```

Excluding the missing `100.0`, the observed values are `{1, 5, 9}` — median
`5`. The excluded value never enters the sort at all, however extreme it is.

Example 4. A slate with no observed values throws.

```cpp
auto all_missing = Slate(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.median();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::median on an empty (all-null) selection
```

## Notes

- Missing entries are excluded before the median is computed.
- Even-length observed sets average their two middle values, using the same
  linear-interpolation convention as `describe()`'s `50%` quartile.
- Always computed in `Float64` (CPU) or `Float32` (off-CPU), regardless of the
  source dtype.
- An empty slate, or one where every entry is missing, throws
  `"Slate::median on an empty (all-null) selection"`.
- Restricted to `Series` payloads; anything else throws `"Slate::median is
  only valid for Series payload"`.

## See Also

- [`Slate::mean`](mean.md): the arithmetic average, more sensitive to
  outliers than the median.
- [`Slate::percentiles`](percentiles.md): the median is exactly the `0.5`
  percentile computed by this method's more general sibling.
- [`Slate::describe`](describe.md): reports this method's result as part of a
  wider numeric summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
