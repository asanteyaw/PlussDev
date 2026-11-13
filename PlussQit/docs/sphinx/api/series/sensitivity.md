<!-- pluss-hand-authored -->

# Series::sensitivity

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sensitivity() const
```

## What It Does

`sensitivity()` returns the current reverse-mode sensitivity associated with
this `Series`.

For tracked leaves after a reverse pass, that returned `Series` is the quantity
most people informally call "the gradient." It has the same logical shape as
the source value and stores how the current objective changed with respect to
each element of that source.

## Read This First

`sensitivity()` always answers one local question:

```text
with respect to this specific Series, what sensitivity is currently stored?
```

That means the same reverse pass can leave different, equally valid numbers on
different nodes.

If `y` was built from `x` and you call

```cpp
y.reverse();
```

then:

- `x.sensitivity()` is `dL/dx`, which for scalar `y` is the familiar `dy/dx`
- `y.sensitivity()` is `dL/dy`, which for scalar `y` as the objective is
  usually `1`

So when users want "the derivative of the result with respect to the input,"
the value they usually mean is read from the tracked input leaf, not from the
result node itself.

There is one subtle but important current-surface behavior to keep in mind:
when no stored sensitivity exists yet, this method returns a same-shaped
zero-valued `Series` rather than throwing.

That is convenient for printing, but it means you should use
[`has_sensitivity()`](has_sensitivity.md) when you need to distinguish "no
stored sensitivity yet" from "a stored sensitivity that happens to be all
zeros."

This method is the read side of the autodiff flow. It does not perform any
propagation itself.

- [`tracks_slope_()`](tracks_slope_.md) decides whether this `Series` should
  receive sensitivity as a tracked leaf.
- [`reverse()`](reverse.md) computes and stores that sensitivity.
- `sensitivity()` reads whatever is currently stored.

## Parameters

None.

## Returns

`Series`
    The stored sensitivity, or a same-shaped zero-valued `Series` when no
    stored sensitivity currently exists.

## Mathematical Meaning

If `x` is a tracked leaf and `L` is the scalar objective used by `reverse()`,
then `x.sensitivity()` stores the elementwise quantity

```text
dL/dx
```

If `x` is a vector or matrix, the returned `Series` has the same shape as `x`,
and each position tells you how a small change at that position would affect
`L`.

When reverse-mode was seeded from a non-scalar output `y`, the effective
objective is

```text
L = sum(seed * y)
```

so the stored sensitivity is really `dL/dx`, not "the derivative of `y`" in an
unqualified sense.

## Where The Stored Result Lives

After a computation such as

```cpp
auto y = f(x);
y.reverse();
```

there are usually several different derivatives in play.

- `x.sensitivity()` answers how the scalar objective changes with respect to
  `x`. For a scalar `y`, that is the familiar `dy/dx`.
- `y.sensitivity()` answers how the scalar objective changes with respect to
  `y` itself. When `y` is the scalar objective, that quantity is usually
  `dL/dy = 1`.

So these two reads are not interchangeable:

- `x.sensitivity()` is where you usually inspect the derivative you care about
  in optimization code
- `y.sensitivity()` is sensitivity stored on the output node itself

This distinction is the main reason a snippet can appear to "return zero" even
when the product rule or chain rule is working correctly underneath.

## Shape And Dtype Notes

- the returned sensitivity follows the source shape
- element positions line up one-for-one with the source positions
- the result is meant for numeric interpretation, not as a boolean state probe

For boolean existence checks, prefer [`has_sensitivity()`](has_sensitivity.md).

## Examples

Example 1. After a reverse pass, `sensitivity()` returns the stored
element-aligned sensitivity.

```cpp
auto x = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
}, pluss::cfg::tracks_slope());

x.mul(3.0).sum().reverse();
std::cout << pluss::series::print(x.sensitivity());
```

The scalar objective here is

```text
L = sum(3x) = 3x₁₁ + 3x₁₂ + 3x₂₁ + 3x₂₂
```

so every partial derivative is `3`. That is why every entry of the stored
sensitivity is `3`.

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 3.0000 3.0000 ⎤
⎣ 3.0000 3.0000 ⎦
```

Example 2. Before any reverse pass has populated one, the current public
surface returns zeros.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << x.sensitivity();
```

This is a storage-state quirk, not a mathematical statement that a reverse
pass already proved the sensitivity is zero.

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 3. Pairing `has_sensitivity()` with `sensitivity()` disambiguates that
state cleanly.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

std::cout << "has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << x.sensitivity();
```

### Output

```text
has_dL_dx=false
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 4. Clearing sensitivity resets the stored value so a later reverse pass
can populate a fresh one.

```cpp
auto reuse = pluss::make_series({1.0, 2.0, 3.0});
reuse.tracks_slope_(true);

reuse.mul(2.0).sum().reverse();
std::cout << "first_dL_dx=" << reuse.sensitivity() << "\n";

reuse.clear();
reuse.mul(5.0).sum().reverse();
std::cout << "second_dL_dx=" << reuse.sensitivity() << "\n";
```

The first objective is `L₁ = 2x₁ + 2x₂ + 2x₃`, so its stored sensitivity is
`[2, 2, 2]`. After clearing, the second objective is
`L₂ = 5x₁ + 5x₂ + 5x₃`, so the new stored sensitivity is `[5, 5, 5]`.

### Output

```text
first_dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
second_dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 5, 5]
```

Example 5. The exact scalar snippet `x * x.cos()` reads back as zero on
`y.sensitivity()` when the input leaf was never marked for tracking.

```cpp
auto x = pluss::make_series(1.7);
auto y = x * x.cos();

y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "x_has_dy_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "y_has_stored_sensitivity=" << (y.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "y_sensitivity=" << y.sensitivity().item<double>() << "\n";
```

The forward value is

```text
y = x cos(x)
```

At `x = 1.7`, that evaluates to `-0.21903564030239187`.

The important point is not the forward value but the storage destination.
`reverse()` can only populate `dy/dx` onto leaves that were opted into
tracking. Since `x` was never marked, there is no stored input sensitivity to
read back.

### Output

```text
y=-0.21903564030239187
x_has_dy_dx=false
y_has_stored_sensitivity=false
y_sensitivity=0
```

Example 6. Once the input is tracked, the same product-rule computation stores
`dy/dx` on `x`, while the scalar output stores `dL/dy = 1`.

```cpp
auto x = pluss::make_series(1.7, pluss::cfg::tracks_slope());
auto y = x * x.cos();

y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
std::cout << "dL_dy=" << y.sensitivity().item<double>() << "\n";
```

Here

```text
y = x cos(x)
```

so the product rule gives

```text
dy/dx = 1·cos(x) + x·(-sin(x))
      = cos(x) - x sin(x)
```

At `x = 1.7`:

```text
dy/dx = cos(1.7) - 1.7 sin(1.7) = -1.8146746720647213
```

Because the reversed objective is the scalar output itself, `L = y`, the
output-node sensitivity is

```text
dL/dy = 1
```

### Output

```text
y=-0.21903564030239187
dy_dx=-1.8146746720647213
dL_dy=1
```

Example 7. Quotient-rule derivatives are read from `sensitivity()` the same
way.

```cpp
auto x = pluss::make_series(1.7);
x.tracks_slope_(true);

auto y = x.div(x.square().add(1.0));
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

The forward function is

```text
y = x / (1 + x²)
```

Differentiate with the quotient rule:

```text
dy/dx = ((1 + x²)·1 - x·2x) / (1 + x²)²
      = (1 - x²) / (1 + x²)²
```

At `x = 1.7`:

```text
dy/dx = (1 - 1.7²) / (1 + 1.7²)² = -0.12490004692012345
```

### Output

```text
y=0.43701799485861187
dy_dx=-0.12490004692012345
```

Example 8. Chain-rule compositions store the full composed derivative on the
tracked input.

```cpp
auto x = pluss::make_series(1.2);
x.tracks_slope_(true);

auto y = x.square().cos();
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

The outer function is `cos(u)` and the inner function is `u = x²`, so

```text
dy/dx = (-sin(u)) · du/dx
      = -sin(x²) · 2x
```

At `x = 1.2`:

```text
dy/dx = -sin(1.2²) · 2(1.2) = -2.3795000356600475
```

### Output

```text
y=0.13042370873814554
dy_dx=-2.3795000356600475
```

Example 9. Hyperbolic and inverse-family methods use the same storage model.

```cpp
auto x = pluss::make_series(0.25);
x.tracks_slope_(true);

auto y = x.atanh();
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

For

```text
y = atanh(x)
```

the derivative is

```text
dy/dx = 1 / (1 - x²)
```

At `x = 0.25`:

```text
dy/dx = 1 / (1 - 0.25²) = 1.0666666666666667
```

### Output

```text
y=0.2554128118829953
dy_dx=1.0666666666666667
```

Example 10. Special functions also write their derivatives into
`sensitivity()`.

```cpp
auto x = pluss::make_series(0.4);
x.tracks_slope_(true);

auto y = x.erf();
y.reverse();

std::cout << "y=" << y.item<double>() << "\n";
std::cout << "dy_dx=" << x.sensitivity().item<double>() << "\n";
```

For

```text
y = erf(x)
```

the derivative is

```text
dy/dx = (2 / √π) exp(-x²)
```

At `x = 0.4`:

```text
dy/dx = (2 / √π) exp(-0.4²) = 0.9615412988393078
```

### Output

```text
y=0.42839235504666845
dy_dx=0.9615412988393078
```

Example 11. Scalar objectives can target selected positions directly rather
than reducing the whole series.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto objective = 2.0 * x.pos(0) + 4.0 * x.pos(1) + x.pos(2).sqrt();
objective.reverse();

std::cout << "L=" << objective.item<double>() << "\n";
pluss::series::print(std::cout, x.sensitivity()) << "\n";
```

The scalar objective is

```text
L = 2x₁ + 4x₂ + √x₃
```

Differentiate entry by entry:

```text
dL/dx₁ = 2
dL/dx₂ = 4
dL/dx₃ = 1 / (2√x₃)
```

At `x = [1, 2, 3]`:

```text
dL/dx₃ = 1 / (2√3) = 0.28867513459481287
```

### Output

```text
L=11.732050807568877
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 4, 0.28867513459481287]
```

Example 12. A mixed scalar objective can combine trigonometric,
constant-weighted, and logarithmic terms.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto objective = x.pos(0).sin() - 1.5 * x.pos(1).cos() + x.pos(2).log();
objective.reverse();

std::cout << "L=" << objective.item<double>() << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

The objective is

```text
L = sin(x₁) - 1.5 cos(x₂) + log(x₃)
```

Differentiate each position:

```text
dL/dx₁ = cos(x₁)
dL/dx₂ = 1.5 sin(x₂)
dL/dx₃ = 1 / x₃
```

At `x = [1, 2, 3]`:

```text
dL/dx₁ = cos(1) = 0.5403023058681398
dL/dx₂ = 1.5 sin(2) = 1.3639461402385225
dL/dx₃ = 1 / 3 = 0.3333333333333333
```

### Output

```text
L=2.56430352829672
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5403023058681398, 1.3639461402385225, 0.3333333333333333]
```

Example 13. When the scalar objective contains products of subexpressions, the
stored sensitivity reflects those internal product-rule terms too.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto objective = x.pos(0).sin() - 1.5 * x.pos(1).cos() * x.pos(2).log();
objective.reverse();

std::cout << "L=" << objective.item<double>() << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

Here

```text
L = sin(x₁) - 1.5 cos(x₂) log(x₃)
```

The partial derivatives are

```text
dL/dx₁ = cos(x₁)
dL/dx₂ = 1.5 sin(x₂) log(x₃)
dL/dx₃ = -1.5 cos(x₂) / x₃
```

At `x = [1, 2, 3]`:

```text
dL/dx₁ = cos(1) = 0.5403023058681398
dL/dx₂ = 1.5 sin(2) log(3) = 1.4984479907474777
dL/dx₃ = -1.5 cos(2) / 3 = 0.2080734182735712
```

### Output

```text
L=1.5272470275894714
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5403023058681398, 1.4984479907474777, 0.2080734182735712]
```

Example 14. For a non-scalar output, the stored sensitivity belongs to the
seeded scalar objective you chose.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);

auto y = x.mul(x);
y.reverse(pluss::make_series({1.0, 10.0, 100.0}));

std::cout << "dL_dx=" << x.sensitivity() << "\n";
```

The forward output is

```text
y = [x₁², x₂², x₃²]
```

The seed defines the scalar objective

```text
L = 1·y₁ + 10·y₂ + 100·y₃
  = 1·x₁² + 10·x₂² + 100·x₃²
```

So the stored sensitivity is

```text
dL/dx₁ = 1 · 2x₁ = 2
dL/dx₂ = 10 · 2x₂ = 40
dL/dx₃ = 100 · 2x₃ = 600
```

### Output

```text
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 40, 600]
```

## Gotchas And Failure Modes

- `sensitivity()` is a numeric readout, not a presence check.
- On the current verified surface, "no stored sensitivity yet" reads back as a
  zero-valued series instead of an exception.
- If you need to know whether the value was actually populated by reverse-mode,
  call [`has_sensitivity()`](has_sensitivity.md) too.
- The returned value should always be interpreted relative to the scalar
  objective that `reverse()` actually used.
- `y.sensitivity()` is not a synonym for "the derivative of `y` with respect to
  the inputs." It is the sensitivity stored on `y` itself.
- The exact snippet `x = make_series(1.7); y = x * x.cos(); y.reverse();`
  raises because `x` was never marked for tracking, so `y` has no connected
  graph. Opt `x` in first (`x.tracks_slope_(true)` or
  `pluss::cfg::tracks_slope()`).
- A zero-valued readback can therefore mean either "the derivative is actually
  zero" or "no stored sensitivity exists yet." Use
  [`has_sensitivity()`](has_sensitivity.md) to distinguish those states.

## Summary

Use `sensitivity()` to read the current reverse-mode sensitivity in series
form. It answers how the scalar objective used by `reverse()` changes with
respect to this specific `Series`, whether that `Series` is an input leaf, an
intermediate, or the output itself. In practice, most optimization code reads
tracked leaves such as `x.sensitivity()`, and pairs that read with
`has_sensitivity()` whenever presence-versus-zero matters.

## See Also

- [`Series::has_sensitivity`](has_sensitivity.md): boolean presence test.
- [`Series::reverse`](reverse.md): populate the stored sensitivity.
- [`Series::clear`](clear.md): clear the stored value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
