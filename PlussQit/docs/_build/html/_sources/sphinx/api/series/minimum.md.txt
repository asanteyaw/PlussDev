<!-- pluss-hand-authored -->

# Series::minimum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::minimum(const Series& rhs) const -> Series
Series::minimum(double rhs) const -> Series
```

## What It Does

Returns the elementwise minimum between `self` and `rhs`.

This is the value-wise mirror of [`maximum`](maximum.md): at each broadcasted
position, the smaller side wins.

Like `maximum()`, this is not a reduction. It preserves the broadcasted shape
and chooses one value per position.

## Parameters

**rhs** (`const Series&` or `double`)

The other operand.

For `minimum(const Series& rhs)`:

- `rhs` is broadcast against `self`
- the result dtype is the promoted dtype of the two operands
- both sides are treated as ordinary values, not as distinguished "data" versus
  "bound"

For `minimum(double rhs)`:

- every element is compared against one shared scalar
- the scalar is materialized onto `self`'s lane, so the scalar overload follows
  `self`'s dtype/platform path directly

## Returns

`Series`
    A new series containing the smaller value at each broadcasted position.

## Gradient

`minimum()` is differentiable in both operands.

The backward rule is:

- gradient flows to `self` where `self <= rhs`
- gradient flows to `rhs` where `self > rhs`

The tie rule is intentionally the same as `maximum()`, not the opposite.

```{note} Tie-breaking convention
When `self[i] == rhs[i]`, the full gradient still goes to `self`. The engine
uses one consistent comparison-boundary convention across
`maximum()`/`minimum()`/`clamp_min()`/`clamp_max()` rather than flipping the tie
direction between `max`-like and `min`-like operations.
```

## Examples

Example 1. The `Series` overload broadcasts a row-shaped peer across the leading
dimension and chooses the smaller value at each position.

```cpp
auto grid = pluss::make_series(
    std::vector<double>{1, 5, 3, 4, 2, 8},
    {2, 3});
auto peer = pluss::make_series(std::vector<double>{2, 6, 1}, {3});

std::cout << grid.minimum(peer);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 5, 1],
  [2, 2, 1]]
```

This is the same broadcast pattern as `maximum()`, but now each location keeps
the smaller side.

Example 2. The scalar overload caps every element against one shared ceiling.

```cpp
std::cout << grid.minimum(4.5);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4.5, 3],
  [4, 2, 4.5]]
```

Only positions above `4.5` are pulled down. This is the same numerical idea as
[`clamp_max`](clamp_max.md), but expressed as an ordinary binary comparison.

Example 3. The tie still attributes its derivative to `self`.

```cpp
auto a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
auto b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
a.tracks_slope_(true);
b.tracks_slope_(true);

auto y = a.minimum(b);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

Index `2` is again the decisive boundary case: the output value is shared, but
the derivative still belongs to `a`.

## Notes

- `minimum()` is non-mutating. The in-place sibling `minimum_()` exists, but the
  method documented here returns a fresh value.
- `minimum(x, scalar)` is often the clearer spelling when the scalar acts as a
  conceptual ceiling; [`clamp_max`](clamp_max.md) is the clearer spelling when
  you want to emphasize the one-sided-bound intent.
- This is not `amin`. If you want a reduction over an axis, use the reduction
  surface.

## See Also

- [`Series::maximum`](maximum.md): the elementwise counterpart. Ties still favor
  `self` there too.
- [`Series::clamp_max`](clamp_max.md): one-sided upper bound with the same
  inclusive comparison boundary.
- [`Series::clamp_min`](clamp_min.md): one-sided lower bound from the same
  comparison family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maximum.html">
    <span class="label">Previous</span>
    <span class="title">maximum</span>
  </a>
</div>
