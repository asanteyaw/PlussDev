<!-- pluss-hand-authored -->

# Series::logit

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::logit(double eps) const -> Series
Series& Series::logit_(double eps)
Series& Series::logit_(double eps) const
```

## What It Does

`logit()` is the inverse of [`sigmoid()`](sigmoid.md) — it maps a probability in
`(0, 1)` back to the real-valued score that produced it:

```
logit(p) = log(p / (1 - p))
```

`p = 0.5` maps to `0`, `p` above `0.5` to a positive score, `p` below `0.5` to a
negative one, and the mapping is antisymmetric: `logit(p) = -logit(1 - p)`. As
`p` approaches `0` or `1` the output diverges to `-inf` or `+inf`.

Because real data often contains exact `0` and `1`, `logit()` takes an `eps`
and clamps `p` to `[eps, 1 - eps]` before the formula. That turns the
infinities into large finite values instead.

## Parameters

**eps** (`double`)

Clamp bound. `p` is restricted to `[eps, 1 - eps]` first. Required. Pass a tiny
value such as `1e-6` to keep exact `0` / `1` inputs finite; pass `0` to allow
`+/-inf` through.

## Returns

`Series` or `Series&`
    The real-valued scores. `logit_()` overwrites the current storage.

## Gradient

`logit()` is differentiable:

```
d/dp logit(p) = 1 / (p * (1 - p))
```

The derivative is smallest (`4`) at `p = 0.5` and grows without bound as `p`
approaches either end. A tracked `logit()` on probabilities near `0` or `1`
therefore produces very large sensitivities.

## In-Place Form

`logit_()` is the in-place sibling of [`logit()`](logit.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `logit_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0.0, 0.5, 1.0}, {3});
x.logit_(1e-6);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-13.81551, 0, 13.81551]
```

## Examples

Example 1. Symmetric probabilities map to symmetric scores.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.1, 0.25, 0.5, 0.75, 0.9}).logit(1e-7));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[-2.1972, -1.0986, 0.0000, 1.0986, 2.1972]
```

`logit(0.5) = 0`, and `logit(0.25) = -logit(0.75)`.

Example 2. `eps` keeps exact `0` and `1` finite by clamping first.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.0, 1.0}).logit(1e-6));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-13.8155, 13.8155]
```

A smaller `eps` pushes those values further out — `logit(0.0)` with
`eps = 1e-12` is about `-27.6`.

Example 3. The backward is `1 / (p * (1 - p))` — minimal at `p = 0.5`.

```cpp
auto p = pluss::make_series({0.2, 0.5, 0.8});
p.tracks_slope_(true);

p.logit(1e-9).sum().reverse();

pluss::series::print(std::cout, p.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[6.2500, 4.0000, 6.2500]
```

`1 / (0.5 * 0.5) = 4` at the center; `1 / (0.2 * 0.8) = 6.25` on either side.

## Gotchas And Failure Modes

- `eps` has no default. Choose it deliberately: too large distorts scores near
  the ends, too small (or `0`) lets `+/-inf` through.
- Inputs outside `(0, 1)` are clamped to `[eps, 1 - eps]`; `logit()` does not
  reject them.
- The gradient blows up near `p = 0` and `p = 1`.

## Summary

`logit(p, eps)` is the inverse sigmoid `log(p / (1 - p))`:

- clamps `p` to `[eps, 1 - eps]` first; `eps` is required
- antisymmetric about `p = 0.5`, where it is `0`
- differentiable, backward `1 / (p * (1 - p))`, unbounded at the ends

## See Also

- [`Series::sigmoid`](sigmoid.md): the forward map `logit()` inverts.
- [`Series::log`](log.md): the logarithm in the definition.
- [`Series::clamp_min`](clamp_min.md) / [`Series::clamp_max`](clamp_max.md): the
  clamping `eps` performs, done explicitly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logical_xor.html">
    <span class="label">Previous</span>
    <span class="title">logical_xor</span>
  </a>
  <a class="next-link" href="logsumexp.html">
    <span class="label">Next</span>
    <span class="title">logsumexp</span>
  </a>
</div>
