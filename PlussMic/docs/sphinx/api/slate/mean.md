<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: dcb835367051 -->

# Slate::mean

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateValue Slate::mean() const
```

## What It Does

Returns the arithmetic mean of every observed (non-missing) value in a numeric
`Series` slate, as a scalar `SlateValue`. Missing entries are excluded before
the mean is computed — both from the sum and from the count used to divide it,
so a missing entry never drags the average toward zero. The computation is
carried out in `Float64` (or `Float32` on a non-CPU platform), regardless of
the source dtype, so an integer-dtype slate still produces a proper fractional
mean rather than a truncated integer result. Restricted to `Series` payloads;
every other kind throws. An empty slate, or one where every entry is missing,
also throws.

See [`Slate::sum`'s note on printing a `SlateValue`](sum.md) — every example
below uses `.to_dbl()` to extract a clean number instead of streaming the
`SlateValue` directly.

## Parameters

`None`

## Returns

`SlateValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the mean of the observed
    values.

## Examples

Example 1. Mean of a numeric slate with no missing entries.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.mean();
std::cout << out.to_dbl();
```

### Output

```text
20
```

Example 2. Missing entries are excluded from both the sum and the divisor.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true})
);
auto out2 = with_gap.mean();
std::cout << out2.to_dbl();
```

### Output

```text
25
```

Only `10.0` and `40.0` participate: `(10 + 40) / 2 = 25`, not `(10 + 40) / 3`.

Example 3. A slate with no observed values throws.

```cpp
auto all_missing = Slate(
    pluss::make_series({10.0, 20.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.mean();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::mean on an empty (all-null) selection
```

Example 4. Any non-`Series` payload throws.

```cpp
auto words = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = words.mean();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::mean is only valid for Series payload
```

## Notes

- Missing entries are excluded from both the numerator and the divisor.
- Always computed in `Float64` (CPU) or `Float32` (off-CPU), regardless of the
  source dtype — an integer slate's mean is a proper fraction, not a truncated
  integer.
- An empty slate, or one where every entry is missing, throws `"Slate::mean
  on an empty (all-null) selection"`.
- Restricted to `Series` payloads; anything else throws `"Slate::mean is only
  valid for Series payload"`.

## See Also

- [`Slate::sum`](sum.md): the total this method divides by the observed
  count.
- [`Slate::median`](median.md): the middle value, less sensitive to outliers
  than the mean.
- [`Slate::std`](std.md) / [`Slate::var`](var.md): spread around this mean.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="median.html">
    <span class="label">Next</span>
    <span class="title">median</span>
  </a>
</div>
