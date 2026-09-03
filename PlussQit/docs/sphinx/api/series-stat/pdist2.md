<!-- pluss-hand-authored -->

# SeriesStatAccessor::pdist2

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::pdist2(const Series& other) const
Series SeriesStatProxy::pdist2() const
Series SeriesStatProxy::pdist2(const Series& other) const
```

## What It Does

`pdist2()` computes the matrix of Euclidean distances between rows of two
`[points,features]` inputs. On this accessor it is a naming companion to
`cdist()`: the `2` refers to two point sets, not to squared distance.

For owner `[m,d]` and peer `[n,d]`, result `[m,n]` has
`out[i,j]=sqrt(sum_k (owner[i,k]-peer[j,k])^2)`.

## Calling Styles

`left.stat.pdist2(right)` and `left.stat(right).pdist2()` are equivalent. The
second spelling uses a peer-bound `SeriesStatProxy`; it does not square the
result or reverse operand order.

## Example

```cpp
auto left = pluss::make_series({{0.0, 0.0}, {1.0, 1.0}});
auto right = pluss::make_series({{1.0, 0.0}, {3.0, 4.0}});
std::cout << left.stat.pdist2(right) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 5],
  [1, 3.6055513]]
```

The bottom-right value is `sqrt((1-3)^2+(1-4)^2)=sqrt(13)`, not `13`.

Example 2. The proxy form binds the second point table before selecting the
method.

```cpp
std::cout << left.stat(right).pdist2() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 5],
  [1, 3.6055513]]
```

Use this form when a program passes around a peer-bound statistical proxy. For
an immediate calculation, `left.stat.pdist2(right)` is shorter and equally
clear.

Example 3. Compare `pdist2()` with a genuinely squared result.

```cpp
auto distance = left.stat.pdist2(right);
std::cout << distance << "\n";
std::cout << distance.square() << "\n";
```

### Output

```text
data=[[1, 5],
  [1, 3.6055513]]
data=[[1, 25],
  [1, 13]]
```

If an objective needs squared Euclidean distance, square the returned matrix
explicitly. That makes both units and gradient scaling visible in the code.

## Gradient

Both point sets participate in reverse mode. Each pair contributes opposite
unit-vector sensitivities to its left and right points. Coincident points use a
zero subgradient; see [`cdist`](cdist.md) for a complete both-sided reverse
example.

## Gotchas And Failure Modes

- `pdist2()` does not return squared distances.
- Both inputs must be rank 2 with matching feature widths and platforms.
- Output shape is `[left rows,right rows]`; it is not a condensed triangular
  vector of within-table distances.
- Use `cdist()` when that name communicates the operation more clearly to your
  readers; the accessor behavior is equivalent.

## See Also

- [`cdist`](cdist.md): the same pairwise Euclidean distance contract.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pdf.html"><span class="label">Previous</span><span class="title">pdf</span></a>
  <a class="next-link" href="skewness.html"><span class="label">Next</span><span class="title">skewness</span></a>
</div>
