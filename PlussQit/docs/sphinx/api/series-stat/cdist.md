<!-- pluss-hand-authored -->

# SeriesStatAccessor::cdist

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::cdist(const Series& other) const
Series SeriesStatProxy::cdist() const
Series SeriesStatProxy::cdist(const Series& other) const
```

## What It Does

`cdist()` returns every Euclidean distance between rows of two point tables.
If the owner is `[m,d]` and `other` is `[n,d]`, output `[m,n]` contains the
distance from owner row `i` to peer row `j`.

## Two Equivalent Binary Call Styles

```cpp
auto direct = left.stat.cdist(right);
auto bound = left.stat(right).cdist();
```

The accessor form passes the peer to the method. The proxy form binds the peer
when `stat(right)` is created. Both keep `left` as the first point set and
produce the same matrix. Prefer the direct form unless a peer-bound chain reads
more clearly.

## Shape Contract

Both operands must be rank 2 and have equal feature width `d`. Row counts may
differ. Dtypes promote to a real floating type and platforms must match.

## Gradient

For nonzero pair difference `a_i-b_j`, its distance derivative is the unit
vector `(a_i-b_j)/||a_i-b_j||`. Reverse mode accumulates contributions from
every pair into both point tables.

## Examples

```cpp
auto left = pluss::make_series({{0.0, 0.0}, {1.0, 1.0}});
auto right = pluss::make_series({{1.0, 0.0}, {3.0, 4.0}});
std::cout << left.stat.cdist(right) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 5],
  [1, 3.6055513]]
```

Rows index points from `left`; columns index points from `right`.

Example 2. Direct and peer-bound proxy calls are value-identical.

```cpp
auto direct = left.stat.cdist(right);
auto bound = left.stat(right).cdist();
std::cout << direct << "\n" << bound << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 5],
  [1, 3.6055513]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 5],
  [1, 3.6055513]]
```

Binding is useful when code chooses a terminal statistic after selecting the
peer. It does not make `right` the owner and does not transpose the matrix.

Example 3. Reverse mode accumulates every pair into both tables.

```cpp
left.tracks_slope_(true);
right.tracks_slope_(true);
left.stat.cdist(right).sum().reverse();
std::cout << left.sensitivity() << "\n";
std::cout << right.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[-1.6, -0.8],
  [-0.5547002, 0.16794971]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, -1],
  [1.1547002, 1.6320503]]
```

Every pair contributes opposite directions to its endpoints, so the sum of all
left and right sensitivities is zero component-wise. This is the translation
invariance expected from distances.

Example 4. Coincident points use a finite zero subgradient.

```cpp
auto a = pluss::make_series({{1.0, 2.0}});
auto b = pluss::make_series({{1.0, 2.0}});
a.tracks_slope_(true);
b.tracks_slope_(true);
a.stat.cdist(b).sum().reverse();
std::cout << a.sensitivity() << "\n" << b.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0, 0]]
pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[-0, -0]]
```

The Euclidean norm has no unique direction at zero. Returning zero is the
stable symmetric convention and prevents a coincident pair from introducing
NaNs into an otherwise valid reverse pass.

## Gotchas And Failure Modes

- Rows are observations and columns are features.
- Output axes are `[owner row, peer row]`; swapping operands transposes the
  distance matrix when the point counts permit comparison.
- Feature-width mismatch fails instead of broadcasting.
- Distance at coincident points is nondifferentiable; treat its zero subgradient
  as a convention rather than a unique geometric direction.

## See Also

- [`pdist2`](pdist2.md): equivalent Euclidean pairwise-distance surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cdf.html"><span class="label">Previous</span><span class="title">cdf</span></a>
  <a class="next-link" href="center.html"><span class="label">Next</span><span class="title">center</span></a>
</div>
