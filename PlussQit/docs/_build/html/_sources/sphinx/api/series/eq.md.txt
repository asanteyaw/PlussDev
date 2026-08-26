<!-- pluss-hand-authored -->

# Series::eq

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::eq(const Series& rhs) const -> Series
Series::eq(double rhs) const -> Series
```

## What It Does

Returns an elementwise equality mask.

`eq()` compares `self` and `rhs` position by position after broadcasting and
numeric type reconciliation have been resolved. The result is not a numeric
sum, difference, or reduction. It is a boolean `Series` whose elements are
`true` exactly where the compared values are equal and `false` everywhere
else.

This makes `eq()` a foundational mask-building method. You use it when you
want to find exact matches, gate other operations, or build conditions for
selection-style methods later in a pipeline.

## Overload Map

`eq(rhs)`

Compares against another `Series`, including scalar-shaped or broadcastable
right-hand sides.

`eq(scalar)`

Compares every element in `self` against one scalar value.

`self == rhs` and `self == scalar`

Operator sugar for the same comparison surface.

## Parameters

**rhs** (`const Series&` or scalar)

The right-hand side to compare against.

When `rhs` is a `Series`, it may be:

- the same shape as `self`
- broadcastable to the shape of `self`
- scalar-shaped

When `rhs` is a scalar, that scalar is compared against every element of
`self`.

## Returns

`Series`
    A new boolean `Series` with the broadcasted comparison shape.

## Gradient

`eq()` is a mask-producing comparison, not a numerically meaningful
differentiable transform. In practice, you teach and use it as control logic:
build a boolean mask, then feed that mask into selection, replacement, or
filtering operations.

## Examples

Example 1. Compare every element to one scalar.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.eq(3.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false  true ⎤
⎣ false false false ⎦
```

Example 2. A row-shaped right-hand side broadcasts across the leading
dimension.

```cpp
auto row = pluss::make_series({1.0, 2.0, 0.0});
std::cout << base.eq(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, false],
  [false, false, false]]
```

Example 3. `==` is just the operator spelling of the same mask-producing
comparison.

```cpp
std::cout << (base == row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, false],
  [false, false, false]]
```

Example 4. Comparison still works across mixed numeric dtypes; the result is
still boolean.

```cpp
auto ints = pluss::series::arange(1, 5, 1, pluss::ScalarType::Int32);
auto floats = pluss::make_series({1.0, 2.0, 3.0, 4.5}).to(pluss::ScalarType::Float32);

std::cout << ints.eq(floats);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, false]
```

Example 5. `eq()` performs exact equality. `NaN` does not compare equal to
`NaN`.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.eq(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

Example 6. Non-broadcastable shapes fail rather than guessing.

```cpp
try {
  std::cout << base.eq(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- `eq()` is exact. It does not apply tolerance, rounding, or "close enough"
  logic for floating values.
- `NaN == NaN` is `false`, as shown above.
- The result dtype is boolean, even when both inputs were numeric.
- Comparison uses the normal broadcast rules. Incompatible shapes throw.

## Teaching Summary

Use `eq()` when you want an exact-match mask, not when you want a reduction and
not when you want approximate numeric closeness. The core mental model is
simple: broadcast first, compare second, then carry the resulting boolean mask
into the next stage of your pipeline.

## See Also

- [`Series::ne`](ne.md): the inequality counterpart.
- [`Series::le`](le.md): inclusive less-than comparison.
- [`Series::ge`](ge.md): inclusive greater-than comparison.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
