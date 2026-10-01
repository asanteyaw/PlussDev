<!-- pluss-hand-authored -->

# Series::acos

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::acos() const -> Series
Series& Series::acos_()
```

## What It Does

Applies the inverse cosine elementwise.

Each value is interpreted as a cosine value and mapped back to its principal
angle in radians. The principal range is `[0, pi]`, so the output is always a
real angle inside that interval when the input stays inside the valid domain
`[-1, 1]`.

This is a pure elementwise transform:

- shape is preserved
- each element is handled independently
- no broadcasting is involved

The important constraint is the domain. `acos()` is only real-valued on
`[-1, 1]`. Values outside that interval surface as `nan` in the verified path
shown below.

## Parameters

None.

This is a zero-argument transform on `self`.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its principal inverse cosine in radians.

## Gradient

Mathematically, `acos(x)` has derivative `-1 / sqrt(1 - x^2)` for interior
points `|x| < 1`.

Two practical consequences follow from that:

- the derivative is negative across the valid interior
- the magnitude grows sharply as `x` approaches `-1` or `1`

On the current verified public surface used for these examples, the reverse-mode
example below returns zero sensitivities rather than the mathematical derivative.
So `acos()` should currently be treated as value-correct, but backward support
is not yet reliable through this method path.

## In-Place Form

`acos_()` is the in-place sibling of [`acos()`](acos.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

```cpp
auto x = pluss::make_series({-0.5, 0.0, 0.5});

auto& ref = x.acos_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0943951, 1.5707963, 1.0471976]
same object: true
```

## Examples

Example 1. Standard anchor values make the angle mapping easy to read.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 0.5, 1.0}).acos());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.1416, 1.5708, 1.0472, 0.0000]
```

Example 2. Matrices keep their shape; only the values become angles.

```cpp
std::cout << pluss::make_series({
    {-1.0, -0.5},
    {0.5, 1.0},
}).acos();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[3.1415927, 2.0943951],
  [1.0471976, 0]]
```

Example 3. The current reverse-mode path returns zero sensitivities on this
surface, which is an important present-day limitation to know.

```cpp
auto x = pluss::make_series({-0.5, 0.0, 0.5});
x.tracks_slope_(true);

auto out = x.acos();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0943951, 1.5707963, 1.0471976]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Values outside `[-1, 1]` are out of domain and surface as `nan`.

```cpp
std::cout << pluss::make_series({-1.25, -1.0, 0.0, 1.0, 1.25}).acos();
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[nan, 3.1415927, 1.5707963, 0, nan]
```

## Gotchas And Failure Modes

- `acos()` is only real-valued on `[-1, 1]`.
- Endpoints are valid for the forward value, but they are numerically delicate
  because the mathematical derivative blows up there.
- On the current verified surface, do not assume reverse-mode support is wired
  just because the value transform works.

## Summary

Use `acos()` when your inputs represent cosine values and you need principal
angles back in radians. The two checks that matter most are domain discipline
and current backward-support expectations.

## Aliases

**`arccos()`** / **`arccos_()`** — spelling is kept for torch-style naming familiarity.
Each resolves to exactly the same dispatch as `acos()` / `acos_()`
before any computation runs, so behavior, gradient, and failure modes are
identical; nothing here is a separate implementation to learn.

## See Also

- [`Series::asin`](asin.md): inverse sine on the same `[-1, 1]` domain.
- [`Series::atan`](atan.md): inverse tangent with no finite-domain restriction.
- [`Series::cos`](cos.md): forward cosine transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs2.html">
    <span class="label">Previous</span>
    <span class="title">abs2</span>
  </a>
  <a class="next-link" href="acosh.html">
    <span class="label">Next</span>
    <span class="title">acosh</span>
  </a>
</div>
