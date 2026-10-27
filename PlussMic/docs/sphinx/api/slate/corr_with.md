<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9f41bd0f64a5 -->

# Slate::corr_with

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::corr_with(const Slate& other, SlateCorrMethod method = SlateCorrMethod::Pearson, std::int64_t min_count = 1) const -> SlateValue
```

## What It Does

Computes the same correlation [`corr()`](corr.md) does, but returns it as a
scalar `SlateValue` gated by `min_count` instead of a raw `double` — below
`min_count` valid pairs, the result is marked *invalid* rather than returning
`NaN` as a plain, unflagged value. Both slates must be numeric `Series`; case
labels align the pairing exactly as they do for [`corr()`](corr.md).

```{note} invalid whenever the underlying correlation isn't finite
Beyond the `min_count` gate, the result is also marked invalid whenever the
underlying [`corr()`](corr.md) computation itself doesn't come out to a finite
number — for example, zero variance on either side (every observed value
tied, under any of `Pearson`/`Spearman`/`KendallTau`) produces a `NaN`
correlation, and that `NaN` is caught and reflected in
`.slate().notnull()` rather than silently passing through as a "valid"
result. You never need to separately guard against a `NaN` hiding behind a
`true` validity flag — check the scalar validity mask and trust it.
```

## Parameters

**other** (`const Slate&`)
    The numeric `Series` slate to correlate with.

**method** (`SlateCorrMethod`, default `Pearson`)
    `Pearson`, `Spearman`, or `KendallTau` — same semantics as
    [`corr()`](corr.md).

**min_count** (`std::int64_t`, default `1`)
    The smallest number of valid pairs still considered valid. Must be `>=
    1`, or it throws.

## Returns

`SlateValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the correlation
    coefficient.

## Examples

Example 1. Same computation as `corr()`, wrapped as a `SlateValue`.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Slate(pluss::make_series({10.0, 10.0, 35.0, 45.0}));
auto out = scores.corr_with(other, SlateCorrMethod::Pearson, 1);
std::cout << out.to_dbl();
```

### Output

```text
0.943119
```

Example 2. Below `min_count` valid pairs, the result is marked invalid.

```cpp
auto small = Slate(pluss::make_series({1.0, 2.0}));
auto small_other = Slate(pluss::make_series({3.0, 4.0}));
auto out2 = small.corr_with(small_other, SlateCorrMethod::Pearson, 5);
std::cout << out2.slate().notnull().as_series().host<bool>()[0];
```

### Output

```text
false
```

Example 3. Zero variance on the Pearson path is caught by the validity check
— the scalar validity mask is `false`, not just a `NaN` value hiding behind
`true`.

```cpp
auto constant = Slate(pluss::make_series({5.0, 5.0, 5.0}));
auto varying = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out3 = constant.corr_with(varying, SlateCorrMethod::Pearson, 1);
std::cout << out3.slate().notnull().as_series().host<bool>()[0];
```

### Output

```text
false
```

Example 4. The same zero-variance case under `KendallTau` — the underlying
rank correlation is `NaN` (every rank on the constant side ties), and that's
caught too, not just the Pearson path.

```cpp
auto out4 = constant.corr_with(varying, SlateCorrMethod::KendallTau, 1);
std::cout << out4.slate().notnull().as_series().host<bool>()[0];
```

### Output

```text
false
```

## Notes

- `min_count` must be `>= 1`, or it throws `"Slate::corr_with: min_count must
  be >= 1"`.
- Case-label alignment applies exactly as it does for [`corr()`](corr.md).
- See the important callout above: validity covers both the `min_count` gate
  and whether the underlying correlation itself came out finite, across all
  three methods.
- Both slates must already share the same platform, or it throws
  `"Slate::corr_with: operands must share a platform"`.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::corr_with: expected a real numeric Series Slate"`.

## See Also

- [`Slate::corr`](corr.md): the same computation, returned as a raw `double`
  with `NaN` on failure instead of a validity-gated `SlateValue`.
- [`Slate::covar`](covar.md): the unnormalized counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr.html">
    <span class="label">Previous</span>
    <span class="title">corr</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
