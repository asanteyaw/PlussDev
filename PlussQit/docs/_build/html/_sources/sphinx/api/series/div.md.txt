<!-- pluss-hand-authored -->

# Series::div

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::div(const Series& rhs) const -> Series
```

## What It Does

Returns an elementwise quotient.

`div()` divides `self` by `rhs` after broadcasting and dtype promotion have
been resolved. The public surface also accepts scalar RHS calls such as
`x.div(2.0)`.

Division is one of the methods where dtype and value edge cases matter more
than on add or multiply, so it is worth explaining carefully:

- integer-looking inputs do not guarantee an integer-looking output
- division by zero on floating inputs does not throw here; it produces
  `inf`, `-inf`, or `nan`

## Parameters

**rhs** (`const Series&`)

The denominator.

`rhs` may be the same shape as `self`, scalar-shaped, or broadcastable to the
shape of `self`.

## Returns

`Series`
    A new `Series` containing the elementwise result.

## Gradient

For differentiable dtypes, the local derivatives are:

- with respect to `self`: `1 / rhs`
- with respect to `rhs`: `-self / rhs^2`

That makes `div()` a good method for showing why denominator values influence
both forward scale and backward sensitivity.

## Examples

Example 1. A scalar RHS divides every element.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << base.div(2.0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.5, 1, 1.5],
  [2, 2.5, 3]]
```

Example 2. A 1D denominator can broadcast across the leading dimension.

```cpp
std::cout << base.div(pluss::make_series({1.0, 2.0, 4.0}));
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1, 0.75],
  [4, 2.5, 1.5]]
```

Example 3. In the verified build, integer-looking division still promotes to a
floating output.

```cpp
auto div_ints = pluss::series::arange(5, 8, 1, pluss::ScalarType::Int64);
std::cout << div_ints.div(2);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5, 3, 3.5]
```

The same verified promotion happens when the denominator is an integer `Series`
too, not only a raw scalar.

Example 4. Floating division by zero produces IEEE-style special values here.

```cpp
std::cout << pluss::make_series({1.0, -1.0, 0.0}).div(0.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[inf, -inf, nan]
```

Example 5. Reverse-mode sensitivities depend on the denominator.

```cpp
auto div_x = pluss::make_series({2.0, 4.0, 8.0});
auto div_y = pluss::make_series({1.0, 2.0, 4.0});
div_x.tracks_slope_(true);
div_y.tracks_slope_(true);

auto div_out = div_x.div(div_y);
div_out.sum().reverse();

std::cout << "out=" << div_out << "\n";
std::cout << "dx=" << div_x.sensitivity() << "\n";
std::cout << "dy=" << div_y.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.5, 0.25]
dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-2, -1, -0.5]
```

Example 6. Non-broadcastable shapes fail.

```cpp
try {
  std::cout << base.div(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- Division is where users most often assume integer truncation. The verified
  public behavior shown here is floating promotion instead.
- Floating division by zero does not throw here; it yields special floating
  values.
- Denominator magnitude directly affects gradient magnitude.

## Summary

Use `div()` when you want broadcast-aware elementwise quotients, but keep your
eyes on two things: output dtype and denominator edge cases. Those two details
are the difference between "obvious arithmetic" and "a bug that quietly moved
into production."

## See Also

- [`Series::mul`](mul.md): multiplicative counterpart.
- [`Series::pow`](pow.md): exponentiation.
- [`Series::sub`](sub.md): additive inverse-side arithmetic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dist.html">
    <span class="label">Previous</span>
    <span class="title">dist</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
