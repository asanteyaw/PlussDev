<!-- pluss-hand-authored -->

# Series::ne

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ne(const Series& rhs) const
Series Series::ne(double rhs) const
Series& Series::ne_(const Series& rhs) const
```

## What It Does

Returns an elementwise inequality mask.

`ne()` answers the opposite question from `eq()`: where are these values
different? It preserves the broadcasted shape and returns a boolean `Series`
with `true` wherever the compared elements differ and `false` wherever they
match exactly.

This is especially useful when you want to locate changed values, exclude a
sentinel, or build the inverse of an equality mask directly instead of
constructing an `eq()` mask first and inverting it later.

## Overload Map

`ne(rhs)`

Compares against another `Series`.

`ne(scalar)`

Compares every element in `self` against one scalar value.

`self != rhs` and `self != scalar`

Operator sugar for the same comparison.

## Parameters

**rhs** (`const Series&` or scalar)

The right-hand side to compare against.

The `Series` overload accepts:

- same-shaped input
- broadcastable input
- scalar-shaped input

The scalar overload applies one repeated comparison across every element.

## Returns

`Series`
    A new boolean `Series` with the broadcasted comparison shape.

## Gradient

Like the rest of the comparison family, `ne()` is best treated as mask logic,
not as a gradient-carrying numeric transform. Its output is boolean and is most
useful as a condition for downstream selection or filtering.

## In-Place Form

`ne_()` is the in-place sibling of [`ne()`](ne.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `ne_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto y = pluss::make_series({1.0, 5.0, 3.0}, {3});

x.ne_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, false]
```

## Examples

Example 1. Scalar inequality marks every element that is not the chosen value.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.ne(3.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ true true false ⎤
⎣ true true  true ⎦
```

Example 2. Broadcasted comparison works the same way it does for `eq()`.

```cpp
auto row = pluss::make_series({1.0, 2.0, 0.0});
std::cout << base.ne(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, false, true],
  [true, true, true]]
```

Example 3. `!=` is the operator form of the same comparison.

```cpp
std::cout << (base != row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, false, true],
  [true, true, true]]
```

Example 4. `NaN` compares as unequal, even against another `NaN`.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.ne(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, true]
```

Example 5. Shape mismatches still fail at the broadcast stage.

```cpp
try {
  std::cout << base.ne(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- `ne()` is exact inequality, not tolerance-based difference testing.
- `NaN != NaN` is `true`, which can surprise people who expect a
  value-preserving "missing equals missing" rule.
- The return value is a boolean mask, not a numeric residual.
- Incompatible shapes still throw broadcast errors.

## Summary

Use `ne()` when you need the direct "different from" mask. It is the cleanest
way to mark non-matching elements without building a numeric difference and
interpreting that by hand.

## Aliases

**`not_equal()`** / **`not_equal_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `ne()` / `ne_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::eq`](eq.md): exact equality.
- [`Series::lt`](lt.md): strict less-than comparison.
- [`Series::gt`](gt.md): strict greater-than comparison.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="narrow_copy.html">
    <span class="label">Previous</span>
    <span class="title">narrow_copy</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
