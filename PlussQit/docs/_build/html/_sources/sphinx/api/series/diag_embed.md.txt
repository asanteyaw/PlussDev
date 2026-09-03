<!-- pluss-hand-authored -->

# Series::diag_embed

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::diag_embed() const -> Series
Series::diag_embed(std::int64_t diagonal) const -> Series
```

## What It Does

`diag_embed()` builds a diagonal matrix from the input values.

The current verified root behavior is important here: the input is flattened
first, then those flattened values are written onto the requested diagonal of a
new square matrix. So on this public surface, `diag_embed()` is not preserving
outer structure. It is operating on a flattened value list.

That makes the method very predictable once you know the rule:

- collect values in flattened order
- allocate a square matrix big enough for the chosen offset
- write one value per diagonal position
- fill everything else with zero

## Overload Map

**`diag_embed()`**

Writes onto the main diagonal.

**`diag_embed(std::int64_t diagonal)`**

Shifts the write target:

- positive values place the diagonal above the main diagonal
- negative values place it below the main diagonal

## Parameters

**diagonal** (`std::int64_t`)

The diagonal offset.

If the flattened input contains `n` values, the output side length becomes
`n + abs(diagonal)`.

## Returns

`Series`
    A square 2D `Series` whose requested diagonal contains the flattened input
    values.

## Gradient

`diag_embed()` preserves reverse-mode connectivity on the current verified
surface.

Because the forward pass writes the flattened input onto one diagonal of a new
matrix, the reverse pass reads sensitivity back from that same diagonal.

## Examples

Example 1. A 1D input becomes a square diagonal matrix.

```cpp
std::cout << pluss::make_series({4, 5, 6}, pluss::ScalarType::Int64).diag_embed();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[4, 0, 0],
  [0, 5, 0],
  [0, 0, 6]]
```

Example 2. Higher-rank input is flattened first on the current root surface.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64).diag_embed(1);
```

### Output

```text
pluss::Series(shape=[5,5], strides=[5,1], dtype=int64, platform=cpu, numel=25, contiguous=true)
data=[[0, 1, 0, 0, 0],
  [0, 0, 2, 0, 0],
  [0, 0, 0, 3, 0],
  [0, 0, 0, 0, 4],
  [0, 0, 0, 0, 0]]
```

This is one of the most important quirks on the page. A `2 x 2` input does not
become a batch or stack of diagonal matrices here. It becomes the flattened
sequence `1, 2, 3, 4`, embedded on an offset diagonal.

Example 3. Reverse-mode reads sensitivity back from the embedded diagonal.

```cpp
auto diag_embed_grad = pluss::make_series({1.0, 2.0, 3.0});
diag_embed_grad.tracks_slope_(true);
diag_embed_grad.diag_embed().sum().reverse();

std::cout << diag_embed_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

## Gotchas And Failure Modes

- The current root method flattens first. Do not assume outer dimensions are
  preserved.
- Offsets increase the output side length by `abs(diagonal)`.
- Every non-diagonal position is zero-filled.
- Reverse-mode reads sensitivity only from the diagonal positions that were
  written during the forward pass.

## Summary

Use `diag_embed()` when you explicitly want a diagonal matrix built from the
input values. On the current public surface, think of it as a flatten-first,
embed-second operation rather than a structure-preserving one.

## See Also

- [`Series::diag`](diag.md): dual-mode extract-or-embed surface.
- [`Series::diagflat`](diagflat.md): same flatten-first idea under a more
  explicit name.
- [`Series::diagonal`](diagonal.md): extraction-only diagonal surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diag.html">
    <span class="label">Previous</span>
    <span class="title">diag</span>
  </a>
  <a class="next-link" href="diagflat.html">
    <span class="label">Next</span>
    <span class="title">diagflat</span>
  </a>
</div>
