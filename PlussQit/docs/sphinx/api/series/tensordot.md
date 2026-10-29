<!-- pluss-hand-authored -->

# Series::tensordot

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::tensordot(const Series& other, std::int64_t dims = 2) const
Series Series::tensordot(const Series& other,
                         const std::vector<std::int64_t>& dims_self,
                         const std::vector<std::int64_t>& dims_other) const
```

## What It Does

`tensordot()` contracts chosen axes of two `Series`: it multiplies them
element-for-element along the paired axes and sums those axes away. The
result's axes are the receiver's remaining ("free") axes, in their original
order, followed by `other`'s free axes in theirs.

It generalizes the matrix product to any rank and any pairing of axes:

- two vectors contracted over their only axis give a dot product
- a matrix and a vector over one axis give a matrix-vector product
- two matrices over their last/first axes give a matrix product
- no contracted axes at all give the outer product
- every axis of both operands contracted gives a scalar

Internally the contracted axes of the receiver are moved last and those of
`other` are moved first, both are flattened to matrices, one matrix product is
taken, and the free axes are unflattened. That makes the cost one
matrix multiplication plus (at most) two layout copies, and makes the result
differentiable in both operands.

## Overload Map

**`tensordot(other, dims)`**

Contract the **last `dims` axes of the receiver** with the **first `dims` axes
of `other`**, paired in order. `dims = 1` is the matrix product for 2-D
operands, `dims = 2` (the default) contracts two trailing/leading axis pairs,
`dims = 0` is the outer product.

**`tensordot(other, dims_self, dims_other)`**

Contract `dims_self[i]` of the receiver with `dims_other[i]` of `other`. The
two lists must have the same length. Axes may be given in any order and may be
negative.

## Parameters

**other** (`const Series&`)
    The second operand. Should share the receiver's dtype.

**dims** (`std::int64_t`, default `2`)
    How many trailing axes of the receiver pair with leading axes of `other`.
    Must be non-negative and no greater than either operand's rank.

**dims_self**, **dims_other** (`const std::vector<std::int64_t>&`)
    The axes to contract, paired by position. Negative values count from the
    end. Contracted axes must have equal length, with one exception: if one of
    a pair has length 1 it is broadcast against the other.

## Returns

`Series`
    Rank `(self.dim() - n) + (other.dim() - n)` for `n` contracted pairs; a
    scalar `Series` when every axis of both operands is contracted.

## Gradient

`tensordot()` is differentiable in both operands. The gradient of each side
is a `tensordot` of the output sensitivity with the other operand, routed
through the reverse rule of the underlying matrix product and layout changes.

## Examples

Example 1. `dims = 1` on two matrices is the matrix product.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto b = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});

std::cout << a.tensordot(b, 1);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 5],
  [10, 11]]
```

The last axis of `a` (length 3) pairs with the first axis of `b` (length 3).

Example 2. `dims = 0` pairs nothing: the result is the outer product.

```cpp
auto u = pluss::make_series({1.0, 2.0, 3.0});
auto v = pluss::make_series({10.0, 20.0});

auto outer = u.tensordot(v, 0);
std::cout << outer;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20],
  [20, 40],
  [30, 60]]
```

The result has shape `[3, 2]` -- all of `u`'s axes, then all of `v`'s.

Example 3. The default `dims = 2` contracts every axis of two same-shape matrices.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});

std::cout << a.tensordot(b);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=70
```

All axes of both operands are contracted, so the result is the scalar `1*5 + 2*6 + 3*7 + 4*8`.

Example 4. Explicit axis lists contract axes that are not trailing/leading.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0}, {2, 3, 4});
auto b = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0}, {5, 4});

// contract a's axis 2 (length 4) with b's axis 1 (length 4)
auto out = a.tensordot(b, {2}, {1});
std::cout << out.sizes()[0] << " " << out.sizes()[1] << " " << out.sizes()[2] << "\n";
std::cout << out;
```

### Output

```text
2 3 5
pluss::Series(shape=[2,3,5], strides=[15,5,1], dtype=float64, platform=cpu, numel=30, contiguous=true)
data=[[[30, 70, 110, 150, 190],
  [70, 174, 278, 382, 486],
  [110, 278, 446, 614, 782]],
  [[150, 382, 614, 846, 1078],
  [190, 486, 782, 1078, 1374],
  [230, 590, 950, 1310, 1670]]]
```

The free axes are `a`'s `[2, 3]` followed by `b`'s `[5]`, giving shape `[2, 3, 5]`.

Example 5. Negative axes count from the end, and several pairs contract at once.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0}, {2, 3, 4});
auto b = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0}, {3, 4});

// a axes (-2, -1) with b axes (0, 1): both pairs have lengths (3, 4)
std::cout << a.tensordot(b, {-2, -1}, {0, 1});
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[650, 1586]
```

Pairs are matched by position: `-2` with `0`, and `-1` with `1`. Only `a`'s leading axis (length 2) is left.

Example 6. A length-1 axis in a contracted pair is broadcast.

```cpp
auto a = pluss::make_series({1.0, 2.0}, {2, 1});          // [2, 1]
auto b = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}});  // [3, 3]

// a's axis 1 has length 1, b's axis 0 has length 3: b is summed along that axis first.
std::cout << a.tensordot(b, {1}, {0});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[12, 15, 18],
  [24, 30, 36]]
```

Each output row is `a[i,0]` times the column sums of `b`.

Example 7. The gradient flows to both operands.

```cpp
auto a = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto b = pluss::make_series({{5.0, 6.0}, {7.0, 8.0}});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.tensordot(b, 1).sum().reverse();

std::cout << a.sensitivity() << "\n" << b.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[11, 15],
  [11, 15]]
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [6, 6]]
```

For the total of `a b`, each entry of `a`'s gradient is the corresponding row sum of `b`, and each entry of `b`'s gradient is the corresponding column sum of `a`.

Example 8. Mismatched lengths and bad axis lists are rejected.

```cpp
auto a = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto b = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});

auto first_line = [](const std::exception& e) {
  std::string msg = e.what();
  return msg.substr(0, msg.find('\n'));
};
try { a.tensordot(b, 1); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
try { a.tensordot(b, {1, 0}, {0}); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
try { a.tensordot(b, 3); }
catch (const std::exception& e) { std::cout << first_line(e) << "\n"; }
```

### Output

```text
tensordot: contracted dimension sizes must match (or be 1)
tensordot: both dimension lists should have same length
tensordot: dims exceeds the rank of an operand
```

## Gotchas And Failure Modes

- `dims` counts axes, not an axis index: `tensordot(b, 1)` contracts the last
  axis of the receiver with the first of `other`.
- The two lists of the explicit overload must have equal length; each
  contracted pair must have equal length (or one must be 1).
- Name each axis at most once in an axis list.
- The result's free-axis order is fixed (receiver's, then `other`'s). Use
  `permute()` or [`einsum()`](einsum.md) when a different order is needed.
- Operands should share a dtype; convert with `to()` first if they differ.

## Summary

`a.tensordot(b, dims)` / `a.tensordot(b, dims_self, dims_other)` contracts
paired axes into one matrix product:

- `dims = n` pairs the last `n` axes of `a` with the first `n` of `b`
- explicit lists pair any axes, negative axes allowed
- free axes of `a` then `b` form the result; `dims = 0` is the outer product
- differentiable in both operands

## See Also

- [`Series::einsum`](einsum.md): the general letter-based contraction.
- [`Series::bmm`](bmm.md): batched matrix product.
- [`Series::LinearAlgAccessor::matmul`](../series-linearalg/matmul.md): the
  rank-polymorphic matrix product.
- [`Series::permute`](permute.md): reorder the result's axes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tensor_split.html">
    <span class="label">Previous</span>
    <span class="title">tensor_split</span>
  </a>
  <a class="next-link" href="threshold.html">
    <span class="label">Next</span>
    <span class="title">threshold</span>
  </a>
</div>
