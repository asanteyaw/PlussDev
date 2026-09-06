<!-- pluss-hand-authored -->

# Series::le

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::le(const Series& rhs) const -> Series
Series::le(double rhs) const -> Series
```

## What It Does

Returns an elementwise less-than-or-equal mask.

`le()` is the inclusive partner to `lt()`. It still asks whether the left-hand
side is below the right-hand side, but it treats equality as a passing case.
That one boundary rule is the whole point of choosing `le()` over `lt()`, and
it matters whenever you are defining closed intervals or inclusive thresholds.

## Overload Map

`le(rhs)`

Compares against another `Series`.

`le(scalar)`

Applies one inclusive threshold to every element.

`self <= rhs` and `self <= scalar`

Operator sugar for the same comparison.

## Parameters

**rhs** (`const Series&` or scalar)

The right-hand side comparison target.

The `Series` overload accepts:

- same-shaped input
- broadcastable input
- scalar-shaped input

The scalar overload checks every element against one repeated boundary value.

## Returns

`Series`
    A new boolean `Series` with `true` wherever `self` is less than or equal to
    the broadcasted right-hand side.

## Gradient

`le()` is mask logic. Its job is to produce a boolean condition that other
operations can use, not to serve as a numerically smooth transform in a
differentiable pipeline.

## Examples

Example 1. Scalar comparison with an inclusive boundary.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.le(4.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ true  true  true ⎤
⎣ true false false ⎦
```

Example 2. Broadcasted comparison works column-wise after expansion.

```cpp
auto row = pluss::make_series({2.0, 2.0, 5.0});
std::cout << base.le(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, true],
  [false, false, false]]
```

Example 3. `<=` is the operator spelling of the same inclusive comparison.

```cpp
std::cout << (base <= row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, true],
  [false, false, false]]
```

Example 4. Equality now counts as `true`.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).le(2.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, true, false]
```

Example 5. `NaN` still does not participate as an ordered value.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.le(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]
```

Example 6. Incompatible shapes are rejected during broadcasting.

```cpp
try {
  std::cout << base.le(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- The only semantic difference from `lt()` is that equality passes.
- `NaN` still yields `false` for the ordered comparison slot.
- The output is boolean, not a reduced truth value for the whole `Series`.
- Broadcast incompatibility is still a hard error.

## Summary

Use `le()` when your boundary is inclusive. If you are reasoning about interval logic,
this is the closed-boundary counterpart to the open-boundary behavior of
[`Series::lt`](lt.md).

## See Also

- [`Series::lt`](lt.md): strict less-than.
- [`Series::ge`](ge.md): inclusive greater-than-or-equal.
- [`Series::eq`](eq.md): equality-only boundary check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ldexp.html">
    <span class="label">Previous</span>
    <span class="title">ldexp</span>
  </a>
  <a class="next-link" href="leaky_relu.html">
    <span class="label">Next</span>
    <span class="title">leaky_relu</span>
  </a>
</div>
