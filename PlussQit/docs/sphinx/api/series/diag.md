<!-- pluss-hand-authored -->

# Series::diag

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::diag() const
Series Series::diag(std::int64_t diagonal) const
```

## What It Does

`diag()` is a dual-mode method:

- on a 1D input, it embeds the values along a diagonal of a new square matrix
- on a 2D input, it extracts one diagonal as a 1D result

That means rank, not just value type, decides which family of result you get
back.

This is the main thing to internalize before using the method heavily. If your
code wants "always extract," use [`diagonal()`](diagonal.md). If your code
wants "always build a diagonal matrix," use [`diag_embed()`](diag_embed.md) or
[`diagflat()`](diagflat.md).

## Overload Map

**`diag()`**

Uses the main diagonal.

**`diag(std::int64_t diagonal)`**

Shifts the diagonal:

- positive values move upward, toward later columns
- negative values move downward, toward later rows

## Parameters

**diagonal** (`std::int64_t`)

The diagonal offset.

For 1D embedding:

- `0` writes onto the main diagonal
- `1` writes one diagonal above the main diagonal
- `-1` writes one diagonal below the main diagonal
- the output side length becomes `input_length + abs(diagonal)`

For 2D extraction:

- `0` reads the main diagonal
- positive offsets read diagonals above the main diagonal
- negative offsets read diagonals below the main diagonal
- the extracted length shrinks naturally when the requested diagonal is shorter

## Returns

`Series`
    The return rank depends on the input rank.

- 1D input returns a 2D square matrix
- 2D input returns a 1D extracted diagonal

Any other input rank is invalid on this root surface.

## Gradient

`diag()` now preserves reverse-mode connectivity on both public branches of the
method.

- in extraction mode, sensitivity is scattered back onto the selected diagonal
- in embedding mode, sensitivity is read back from the embedded diagonal

So the gradient follows the same branch decision that the forward pass uses.

## Examples

Example 1. A 1D input becomes a diagonal matrix.

```cpp
std::cout << pluss::make_series({9, 8, 7}, pluss::ScalarType::Int64).diag();
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[9, 0, 0],
  [0, 8, 0],
  [0, 0, 7]]
```

Example 2. A positive offset grows the matrix and moves the values above the
main diagonal.

```cpp
std::cout << pluss::make_series({9, 8, 7}, pluss::ScalarType::Int64).diag(1);
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=int64, platform=cpu, numel=16, contiguous=true)
data=[[0, 9, 0, 0],
  [0, 0, 8, 0],
  [0, 0, 0, 7],
  [0, 0, 0, 0]]
```

Example 3. A 2D input switches the method into extraction mode.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, square.diag());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 5, 9]
```

Example 4. Negative offsets extract below the main diagonal.

```cpp
std::cout << square.diag(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[4, 8]
```

Example 5. Higher-rank input is not accepted here.

```cpp
try {
  std::cout << pluss::make_series({
      {
          {1, 2},
          {3, 4},
      },
      {
          {5, 6},
          {7, 8},
      },
  }, pluss::ScalarType::Int64).diag() << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
diag/diagonal: extraction expects a 2D input
```

Because `diag()` is dual-mode rather than fully polymorphic across ranks, rank
3 does not mean "apply this batch-wise." It means "this call is invalid."

Example 6. In extraction mode, reverse-mode writes sensitivity back onto the
selected diagonal only.

```cpp
auto diag_grad = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0},
});
diag_grad.tracks_slope_(true);
diag_grad.diag().sum().reverse();

pluss::series::print(std::cout, diag_grad.sensitivity());
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 1.0000 0.0000 0.0000 ⎤
⎢ 0.0000 1.0000 0.0000 ⎥
⎣ 0.0000 0.0000 1.0000 ⎦
```

## Gotchas And Failure Modes

- `diag()` changes role based on input rank. This is convenient, but it also
  means a rank mistake can silently change the kind of result you were
  expecting.
- 1D input embeds; 2D input extracts; rank-0 and rank-3+ inputs are invalid.
- Offsets can change both output shape and diagonal length.
- Reverse-mode follows the active branch: scatter back on extraction, read back
  from the diagonal on embedding.

## Summary

Use `diag()` when the input rank itself should decide whether you extract or
embed. If you need one behavior only, prefer the single-purpose neighbors:
`diagonal()` for extraction, `diag_embed()` or `diagflat()` for embedding.

## See Also

- [`Series::diagonal`](diagonal.md): extraction-only diagonal surface.
- [`Series::diag_embed`](diag_embed.md): explicit embedding surface.
- [`Series::diagflat`](diagflat.md): flatten-then-embed surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="detach_.html">
    <span class="label">Previous</span>
    <span class="title">detach_</span>
  </a>
  <a class="next-link" href="diag_embed.html">
    <span class="label">Next</span>
    <span class="title">diag_embed</span>
  </a>
</div>
