<!-- pluss-hand-authored -->

# Series::mT

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::mT() const
```

## Transpose Every Trailing Matrix

`mT()` swaps the final two axes and leaves every leading axis unchanged. A
rank-2 matrix is transposed; a higher-rank batch has each trailing matrix
transposed independently. This fixed-axis meaning keeps batched matrix formulas
readable without calculating axis numbers at each call site.

The result is a zero-copy stride view. Values stay in the same storage and only
shape/stride metadata changes, so the operation is constant-time in `numel()`.
The result commonly is not contiguous. Use [`contiguous()`](contiguous.md) only
when a later kernel requires packed storage.

`mT()` does not conjugate complex values. Use [`adjoint()`](adjoint.md) for a
conjugate transpose. Use [`transpose()`](transpose.md) when the axes to swap are
not the final two.

## Rank, Shape, And Reverse Mode

Rank must be at least two. For shape `[B0, ..., R, C]`, the result shape is
`[B0, ..., C, R]`; the corresponding final two strides are swapped. Applying
`mT()` twice restores the original geometry and values.

Reverse mode applies the same final-axis swap to incoming sensitivity. Because
the forward is a view, no value kernel or numerical approximation is involved.

## Examples

### 1. Transpose A Matrix As A View

```cpp
auto matrix = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
std::cout << matrix.mT();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

The source strides `[3,1]` become `[1,3]`; no payload moved.

### 2. Preserve Batch Axes

```cpp
auto batch = pluss::make_series(
    std::vector<double>{1,2,3,4,5,6,7,8,9,10,11,12},
    {2, 2, 3});
std::cout << batch.mT();
```

### Output

```text
pluss::Series(shape=[2,3,2], strides=[6,1,3], dtype=float64, platform=cpu, numel=12, contiguous=false)
data=[[[1, 4],
  [2, 5],
  [3, 6]],
  [[7, 10],
  [8, 11],
  [9, 12]]]
```

Only dimensions `1` and `2` exchange roles; batch dimension `0` remains first.

### 3. Transpose Twice

```cpp
auto round_trip = matrix.mT().mT();
std::cout << "equal=" << std::boolalpha << round_trip.equal(matrix)
          << " strides=[" << round_trip.stride(0) << ","
          << round_trip.stride(1) << "]";
```

### Output

```text
equal=true strides=[3,1]
```

### 4. Reverse Mode Swaps The Weight Matrix Back

```cpp
auto x = matrix.clone();
x.tracks_slope_(true);
(x.mT() * pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}}))
    .sum().reverse();
std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 3, 5],
  [2, 4, 6]]
```

### 5. Reject A Vector

```cpp
try { (void)pluss::make_series({1.0, 2.0}).mT(); }
catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
mT: expected a series with at least 2 dimensions, got a 1-D series
```

Scalars fail for the same reason: there are not two axes to exchange.

## See Also

- [`Series::adjoint`](adjoint.md): transpose and conjugate trailing matrices.
- [`Series::transpose`](transpose.md): swap any selected axis pair.
- [`Series::t`](t.md): two-dimensional transpose with different rank rules.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html"><span class="label">Previous</span><span class="title">lt</span></a>
  <a class="next-link" href="masked_fill.html"><span class="label">Next</span><span class="title">masked_fill</span></a>
</div>
