<!-- pluss-hand-authored -->

# Series::ge

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ge(const Series& rhs) const
Series Series::ge(double rhs) const
Series& Series::ge_(const Series& rhs) const
```

## What It Does

Returns an elementwise greater-than-or-equal mask.

`ge()` is the inclusive partner to `gt()`. It marks positions where the
left-hand side is either larger than the right-hand side or exactly equal to
it. That equality rule is the entire difference between the strict and
inclusive forms, and it is the rule that determines whether your lower bound is
open or closed.

## Overload Map

`ge(rhs)`

Compares against another `Series`.

`ge(scalar)`

Applies one inclusive lower bound to every element.

`self >= rhs` and `self >= scalar`

Operator sugar for the same comparison.

## Parameters

**rhs** (`const Series&` or scalar)

The comparison target.

The `Series` overload accepts:

- same-shaped input
- broadcastable input
- scalar-shaped input

The scalar overload compares every element in `self` against one repeated
boundary value.

## Returns

`Series`
    A new boolean `Series` with `true` wherever `self` is greater than or equal
    to the broadcasted right-hand side.

## Gradient

`ge()` is comparison logic that yields a boolean mask. Its practical role is
conditional control, not numeric differentiation.

## In-Place Form

`ge_()` is the in-place sibling of [`ge()`](ge.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `ge_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, {3});
auto y = pluss::make_series({1.0, 3.0, 3.0}, {3});

x.ge_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, true]
```

## Examples

Example 1. Scalar comparison with an inclusive lower bound.

```cpp
auto base = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

pluss::series::print(std::cout, base.ge(3.0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false true ⎤
⎣  true  true true ⎦
```

Example 2. A broadcasted row can provide one lower bound per column.

```cpp
auto row = pluss::make_series({1.0, 2.0, 5.0});
std::cout << base.ge(row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, false],
  [true, true, true]]
```

Example 3. `>=` is the operator spelling of the same inclusive comparison.

```cpp
std::cout << (base >= row);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, true, false],
  [true, true, true]]
```

Example 4. Equality now passes.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).ge(2.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, true]
```

Example 5. `NaN` still does not behave like an ordered numeric value.

```cpp
auto lhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), -1.0});
auto rhs = pluss::make_series({1.0, std::numeric_limits<double>::quiet_NaN(), 0.0});

std::cout << lhs.ge(rhs);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
```

Example 6. Shape mismatches are still broadcast errors.

```cpp
try {
  std::cout << base.ge(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}));
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

- The only semantic difference from `gt()` is that equality counts as `true`.
- `NaN` remains unordered and therefore does not pass the ordered slot.
- The output is boolean and shape-preserving after broadcasting.
- Broadcast incompatibility is still a hard error.

## Summary

Use `ge()` when your lower bound is inclusive. In interval language, it closes
the boundary that [`Series::gt`](gt.md) leaves open.

## Aliases

**`greater_equal()`** / **`greater_equal_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `ge()` / `ge_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::gt`](gt.md): strict greater-than.
- [`Series::le`](le.md): inclusive less-than-or-equal.
- [`Series::eq`](eq.md): equality-only check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gcd.html">
    <span class="label">Previous</span>
    <span class="title">gcd</span>
  </a>
  <a class="next-link" href="geometric_.html">
    <span class="label">Next</span>
    <span class="title">geometric_</span>
  </a>
</div>
