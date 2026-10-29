<!-- pluss-hand-authored -->

# Series::AntidiffAccessor::cumulative_trapezoid

[Back to Series::AntidiffAccessor API Reference](../../series_antidiff_accessor_api_reference.md)

## Signatures

```cpp
Series Series::AntidiffAccessor::cumulative_trapezoid(const Series& x, std::int64_t dim = -1) const
Series Series::AntidiffAccessor::cumulative_trapezoid(double dx, std::int64_t dim = -1) const
Series Series::AntidiffAccessor::cumulative_trapezoid(std::int64_t dim = -1) const

Series SeriesAntidiffProxy::cumulative_trapezoid(const Series& x, std::int64_t dim = -1) const
Series SeriesAntidiffProxy::cumulative_trapezoid(double dx, std::int64_t dim = -1) const
Series SeriesAntidiffProxy::cumulative_trapezoid(std::int64_t dim = -1) const

Series pluss::subsp::math::integ::cumulative_trapezoid(const Series& y, const Series& x, std::int64_t dim = -1)
Series pluss::subsp::math::integ::cumulative_trapezoid(const Series& y, double dx, std::int64_t dim = -1)
Series pluss::subsp::math::integ::cumulative_trapezoid(const Series& y, std::int64_t dim = -1)
```

## What It Does

`cumulative_trapezoid()` computes the *running* antiderivative of
already-sampled data: the same composite-trapezoidal area
[`trapezoid()`](trapezoid.md) computes, but kept at every partial sum
instead of only the final total. Where `trapezoid()` returns one number (or
one per batch row), `cumulative_trapezoid()` returns one value per sample
after the first — an approximation of `F(x_i) = integral_{x_0}^{x_i} y dx`
for each `i`, in the same sense that `Series::cumsum()` is the running form
of `Series::sum()`.

This is torch parity: `torch.cumulative_trapezoid` is `torch.trapezoid`'s
own running counterpart, and, like `trapezoid`, real torch declares it with
no dedicated kernel — computed compositionally, the same approach taken
here.

## Relationship To `trapezoid()`

`cumulative_trapezoid()`'s **last** entry along `dim` always equals
`trapezoid()`'s result computed over the identical `y`/`x`/`dx` — the total
area is exactly the running sum's final term. This is not a coincidence of
implementation; it follows from what a running sum means, and it is a
useful cross-check when verifying either method independently (see
Example 4, which confirms both methods' gradients agree at that shared
point too).

## Parameters

Identical to [`trapezoid()`](trapezoid.md)'s: **x** (coordinates, optional),
**dx** (uniform spacing, used when `x` is omitted), and **dim** (the axis to
accumulate along, default `-1`). The three calling forms
(`cumulative_trapezoid(x)`, `cumulative_trapezoid(dx)`,
`cumulative_trapezoid()`) resolve exactly the same way `trapezoid()`'s do —
see [`Series::antidiff`](../series/antidiff.md) for the peer-binding rule
behind the zero-argument form.

## Returns

`Series`
    A new `Series` with `dim`'s length reduced by one (the running sum
    starts after the first pair of samples, so an `n`-sample axis produces
    `n - 1` cumulative values) — the same convention `torch.
    cumulative_trapezoid` uses. Leading dimensions are preserved
    (see Batching below).

## Batching

Like `trapezoid()`, the accumulation happens along `dim` specifically,
never a hardcoded axis, so `y` may carry leading batch dimensions sharing
the same `x`; each batch row gets its own independent running sum in one
call. See Example 3.

## Gradient

No custom backward rule exists here either — the gradient is inherited
entirely from the composed `diff`/`slice`/`mul`/`add`/`cumsum` ops. Because
the last cumulative entry equals `trapezoid()`'s own total, differentiating
through *only* that last entry reproduces `trapezoid()`'s exact gradient
(the trapezoid weight of each sample) — Example 4 verifies this directly
rather than asserting it.

## Examples

Example 1. The two binary call styles are equivalent.

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

std::cout << "call_style=" << y.antidiff().cumulative_trapezoid(x) << "\n";
std::cout << "dot_style=" << y.antidiff.cumulative_trapezoid(x) << "\n";
```

### Output

```text
call_style=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 3, 9.5, 22]
dot_style=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 3, 9.5, 22]
```

Note the last entry, `22`, matches [`trapezoid()`](trapezoid.md)'s own
result for this exact `y`/`x` (see that page's Example 1).

Example 2. A uniform `dx` versus the zero-argument default (`dx = 1.0`).

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});

std::cout << "with_dx=" << y.antidiff.cumulative_trapezoid(0.5) << "\n";
std::cout << "no_arg_default=" << y.antidiff.cumulative_trapezoid() << "\n";
```

Halving the spacing halves every running value, exactly as it halves
`trapezoid()`'s own total.

### Output

```text
with_dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.25, 1.5, 4.75, 11]
no_arg_default=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5, 3, 9.5, 22]
```

Example 3. Batching: one call accumulates two independent rows sharing the
same `x`.

```cpp
auto y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 10.0, 20.0, 30.0, 40.0}, {2, 4});
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0});

std::cout << y.antidiff.cumulative_trapezoid(x, -1) << "\n";
```

Each row's last entry matches [`trapezoid()`'s own batched
result](trapezoid.md) for the identical data (`7.5` and `75`).

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1.5, 4, 7.5],
  [15, 40, 75]]
```

Example 4. Differentiating through only the LAST cumulative entry
reproduces `trapezoid()`'s own gradient exactly — an independent
cross-check that the two methods' reverse-mode paths agree, not just their
forward values.

```cpp
auto y = pluss::make_series({0.0, 1.0, 4.0, 9.0, 16.0});
y.tracks_slope_(true);
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0});

auto cum = y.antidiff.cumulative_trapezoid(x);
auto last = cum.select(0, cum.numel() - 1);
last.reverse();

std::cout << "dLast_dy=" << y.sensitivity() << "\n";
```

### Output

```text
dLast_dy=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.5, 1, 1, 1, 0.5]
```

Identical to [`trapezoid()`'s own gradient example](trapezoid.md) for the
same data — `[0.5, 1, 1, 1, 0.5]`, the trapezoid weight of each sample.

Example 5. Complex `y` works with no special handling, same as
`trapezoid()`.

```cpp
auto y = pluss::make_series(std::vector<std::complex<double>>{{0.0, 0.0}, {1.0, 2.0}, {4.0, 8.0}}, {3})
              .to(pluss::ScalarType::Complex128);
auto x = pluss::make_series({0.0, 1.0, 2.0}).to(pluss::ScalarType::Complex128);

std::cout << "complex_cumarea=" << y.antidiff.cumulative_trapezoid(x) << "\n";
```

The last entry, `(3+6j)`, matches
[`trapezoid()`'s own complex example](trapezoid.md).

### Output

```text
complex_cumarea=pluss::Series(shape=[2], strides=[1], dtype=complex128, platform=cpu, numel=2, contiguous=true)
data=[(0.5+1j), (3+6j)]
```

## Gotchas And Failure Modes

- The output has one FEWER entry along `dim` than the input — an
  `n`-sample axis produces `n - 1` cumulative values, matching
  `torch.cumulative_trapezoid`'s own convention. Reaching for element `i`
  of the result gives the area up through sample `i + 1`, not sample `i`.
- The same peer-binding, broadcasting, and descending-`x` behavior as
  [`trapezoid()`](trapezoid.md) applies here — see that page's own Gotchas
  section.
- `cumulative_trapezoid()` is out-of-place; it never mutates `y` or `x`.

## Summary

`cumulative_trapezoid()` is `trapezoid()`'s running form: the same
composite-trapezoidal formula, kept at every partial sum instead of
collapsed to one total. Its last entry always equals `trapezoid()`'s own
result over identical data — use that identity to sanity-check either one
independently.

## See Also

- [`Series::AntidiffAccessor::trapezoid`](trapezoid.md): the total-area form
  of this same computation.
- [`Series::cumsum`](../series/cumsum.md): the plain running sum this
  method's weighted version builds on.
- [`Series::diff`](../series/diff.md): the discrete forward difference the
  `antidiff` accessor reverses.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="trapezoid.html">
    <span class="label">Previous</span>
    <span class="title">trapezoid</span>
  </a>
  <a class="next-link" href="../../series_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series API Reference</span>
  </a>
</div>
