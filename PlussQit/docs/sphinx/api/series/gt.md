<!-- pluss-hand-authored -->

# Series::gt

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::gt(const Series& rhs) const -> Series
Series::gt(double rhs) const -> Series
```

## What It Does

Returns an elementwise strict greater-than mask.

`gt()` is the mirror image of `lt()`: it marks positions where the left-hand
value is strictly larger than the right-hand value. Equality does not pass, so
this is the strict comparison to use when you want an exclusive lower bound.

## Overload Map

`gt(rhs)`

Compares against another `Series`.

`gt(scalar)`

Compares every element in `self` against one scalar threshold.

`self > rhs` and `self > scalar`

Operator sugar for the same strict comparison.

## Parameters

**rhs** (`const Series&` or scalar)

The comparison target.

The `Series` overload accepts:

- same-shaped input
- broadcastable input
- scalar-shaped input

The scalar overload reuses one threshold across every element in `self`.

## Returns

`Series`
    A new boolean `Series` with `true` wherever `self` is strictly greater than
    the broadcasted right-hand side.

## Gradient

`gt()` produces boolean control information. It is not the kind of surface you
use to propagate smooth numeric sensitivity.

## Examples

Example 1. Scalar thresholding marks values above the cutoff.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.gt(3.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false false ⎤
⎣  true  true  true ⎦
```

Example 2. Broadcasted comparisons can use a per-column lower bound.

```cpp
auto row = pluss::make_series({0.0, 2.0, 5.0});
std::cout << base.gt(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, false],
  [true, true, true]]
```

Example 3. `>` is equivalent to `gt()`.

```cpp
std::cout << (base > row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, false],
  [true, true, true]]
```

Example 4. Equality still fails the strict comparison.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).gt(2.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, true]
```

Example 5. `NaN` does not compare as greater-than.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.gt(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, false, false]
```

Example 6. Shapes still need to broadcast cleanly.

```cpp
try {
  std::cout << base.gt(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- `gt()` is strict, so equality yields `false`.
- `NaN` is not ordered, so it does not count as greater-than.
- The result is a boolean mask, not a numeric difference.
- Broadcast rules are unchanged from the rest of the elementwise API.

## Teaching Summary

Use `gt()` for exclusive lower bounds. When the equality case must pass too,
reach for [`Series::ge`](ge.md).

## See Also

- [`Series::ge`](ge.md): inclusive greater-than-or-equal.
- [`Series::lt`](lt.md): strict less-than.
- [`Series::ne`](ne.md): inequality without ordering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
