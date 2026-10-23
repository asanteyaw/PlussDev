<!-- pluss-hand-authored -->

# Series::CalcAccessor::diff.ito

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signatures

```cpp
// One state, one or several Brownian factors
Series::ItoDifferential Series::CalcAccessor::DiffAccessor::ito(
    const std::function<Series(const Series&)>& g,
    const Series& mu, const Series& sigma) const

// ... with a jump part
Series::ItoDifferential ito(g, mu, sigma, const Series::JumpMeasure& jumps) const

// Two states: the receiver and `other`
Series::ItoDifferential ito(
    const std::function<Series(const Series&, const Series&)>& g,
    const Series& mu, const Series& sigma, const Series::ItoState& other) const
Series::ItoDifferential ito(g, mu, sigma, other, const Series::JumpMeasure& jumps) const

// The two named special cases of the two-state form
Series::ItoDifferential ito_product(mu, sigma, other) const     // g(x, y) = x y
Series::ItoDifferential ito_quotient(mu, sigma, other) const    // g(x, y) = x / y

// Any number of states, optional jumps, optional time argument
Series::ItoDifferential ito(const ItoFn& g, mu, sigma,
    const std::vector<Series::ItoState>& others,
    const Series::JumpMeasure* jumps, const Series::ItoOptions& options) const

// A function that depends on time as well: g(t, x)
Series::ItoDifferential ito_at(const Series& time,
    const std::function<Series(const Series&, const Series&)>& g,
    const Series& mu, const Series& sigma) const
```

## What It Computes

If `X` is an Itô process, `dX = μ dt + σ dW`, then a smooth function `g(X)` is
again an Itô process, and Itô's lemma gives its coefficients:

```text
drift      = g'(x) μ + ½ g''(x) σ²
diffusion  = g'(x) σ
```

`ito` computes both without a hand-written formula for `g'` or `g''`. It
differentiates `g` by the same automatic-differentiation engine as
[`diff`](diff.md): one reverse pass for the gradient and, for the second-order
term, one more pass per state. The extra `½ g'' σ²` is the whole difference from
the ordinary chain rule; it exists because the squared increment of a Brownian
path is its elapsed time, not zero.

The receiver holds the states. Every element is an independent point, so one call
evaluates the differential on a whole grid, or for a whole batch of independent
processes. `g` therefore has to act elementwise: the value of `g` at one element
may not depend on another element. `mu` has the receiver's shape.

With several states (`other`, or `others`), the multivariate formula applies. The
second-order term is `½ Σ_ij ∂²g/∂x_i∂x_j · σ_i · σ_j`, so the covariation of two
states enters through the dot product of their volatility rows. That is why all
states in one call must share the same Brownian factors, and why `sigma` of every
state has the same trailing length.

## The Result

`Series::ItoDifferential` has these members:

| Member | Shape | Meaning |
| --- | --- | --- |
| `value` | receiver | `g` at the point |
| `drift` | receiver | the coefficient of `dt` |
| `diffusion` | receiver + `[m]` | the coefficient of each Brownian factor (`m = 1` for a single factor) |
| `jump_sizes` | `[K]` + receiver | `g(x + z_k) − g(x)` for each jump atom; undefined without jumps |
| `jump_intensity` | `[K]` (or `[K]` + receiver) | the rate of each atom; undefined without jumps |
| `jumps_compensated` | bool | whether the jump part has been compensated |

## Parameters

- `g` maps the state (or states) to the function value, elementwise. For the
  general form it receives the states as a `std::vector<Series>`, in the order
  receiver followed by `others`; with `options.time` set, its first element is the
  time.
- `mu` is the drift of the receiver, with the receiver's shape.
- `sigma` is the volatility of the receiver. With one Brownian factor it has the
  receiver's shape. With `m` factors it has the receiver's shape plus a trailing
  axis of length `m`. A state driven by one factor and a state driven by two
  cannot be mixed in one call: give both the same trailing length, using zeros
  for the factor a state does not load on.
- `other` (`Series::ItoState{x, mu, sigma}`) is the second state. `x` has the
  receiver's shape.
- `jumps` (`Series::JumpMeasure{sizes, intensity, compensated}`) is the jump part,
  described below.
- `options.time` is a `Series` of times. When set, `g`'s first argument is the
  time, and the drift gains the partial time derivative `∂g/∂t`.
- `ito_at(time, g, mu, sigma)` is the one-state, time-dependent special case.

### Jumps

A jump of size `z` does not need a derivative: it moves `g(X)` by `g(x + z) −
g(x)`, however rough `g` is. A jump part is therefore given as an *atomic
measure*: atom `k` jumps by `sizes[0][k]` at rate `intensity[k]` per unit time.
A compound Poisson process with rate `λ` and jump law `p_k` has `intensity[k] =
λ p_k`; a Lévy measure with a density is represented by quadrature atoms and
weights.

| `compensated` | `X` contains | Drift of `g(X)` |
| --- | --- | --- |
| `false` | the sum of the jumps | the continuous part only; the jumps are reported in `jump_sizes` |
| `true` | the jumps minus their mean, a martingale | the continuous part plus `Σ ν_k [g(x + z_k) − g(x) − g'(x) z_k]` |

## Examples

Example 1. One state, one Brownian factor. For `g(x) = x²` the lemma gives drift
`2xμ + σ²` and diffusion `2xσ`.

```cpp
auto x = pluss::make_series({0.5, 1.0, -0.3});
auto mu = pluss::make_series({0.1, 0.2, -0.1});
auto sigma = pluss::make_series({0.3, 0.5, 0.2});

auto d = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma);

std::cout << "value     " << d.value << "\n";
std::cout << "drift     " << d.drift << "\n";
std::cout << "diffusion " << d.diffusion << "\n";
```

### Output

```text
value     pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.25, 1, 0.09]
drift     pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.19, 0.65, 0.1]
diffusion pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.3],
  [1],
  [-0.12]]
```

Example 2. Several Brownian factors. Give `sigma` a trailing axis of length `m`;
the second-order term sums the squared loadings and the diffusion gains one column
per factor.

```cpp
auto x = pluss::make_series({0.5, 1.0});
auto mu = pluss::make_series({0.1, 0.2});
auto sigma = pluss::make_series({{0.3, 0.1}, {0.5, 0.2}});  // [2 states, 2 factors]

auto d = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma);

std::cout << "drift     " << d.drift << "\n";
std::cout << "diffusion " << d.diffusion << "\n";
```

### Output

```text
drift     pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.2, 0.69]
diffusion pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.3, 0.1],
  [1, 0.4]]
```

Example 3. A function of two states. The covariation term depends on how the two
volatility rows line up. Here `X` loads only on the first factor and `Y` on both,
so `d[X, Y] = σ_x · σ_y dt`.

```cpp
auto x = pluss::make_series({2.0});
auto mu_x = pluss::make_series({0.05});
auto sigma_x = pluss::make_series({{0.3, 0.0}});

pluss::Series::ItoState y{pluss::make_series({1.5}), pluss::make_series({0.08}),
                          pluss::make_series({{0.24, 0.32}})};

auto g = [](const pluss::Series& a, const pluss::Series& b) { return a.sub(b).pow(2.0); };
auto d = x.calc.diff.ito(g, mu_x, sigma_x, y);

std::cout << "drift     " << d.drift << "\n";
std::cout << "diffusion " << d.diffusion << "\n";
```

### Output

```text
drift     pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.076]
diffusion pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.06, -0.32]]
```

Example 4. `ito_product` and `ito_quotient` are the named special cases. The product
rule is `d(XY) = X dY + Y dX + d[X, Y]`.

```cpp
auto x = pluss::make_series({2.0});
auto mu_x = pluss::make_series({0.05});
auto sigma_x = pluss::make_series({{0.3, 0.0}});
pluss::Series::ItoState y{pluss::make_series({1.5}), pluss::make_series({0.08}),
                          pluss::make_series({{0.24, 0.32}})};

auto product = x.calc.diff.ito_product(mu_x, sigma_x, y);
auto quotient = x.calc.diff.ito_quotient(mu_x, sigma_x, y);

std::cout << "product drift  " << product.drift << "\n";
std::cout << "  X mu_y + Y mu_x + cov = " << 2.0 * 0.08 + 1.5 * 0.05 + 0.3 * 0.24 << "\n";
std::cout << "quotient drift " << quotient.drift << "\n";
```

### Output

```text
product drift  pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.307]
  X mu_y + Y mu_x + cov = 0.307
quotient drift pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.025037037]
```

Example 5. Time-dependent functions go through `ito_at`. Discounting
`g(t, x) = e^(−rt) x` has drift `e^(−rt)(μ − r x)`, which vanishes exactly for a
process growing at the rate `r`.

```cpp
const double r = 0.03;
auto x = pluss::make_series({100.0, 80.0});
auto mu = pluss::make_series({r * 100.0, 0.07 * 80.0});
auto sigma = x.mul(0.2);
auto time = pluss::make_series(0.8);

auto d = x.calc.diff.ito_at(
    time, [r](const pluss::Series& t, const pluss::Series& s) { return s.mul(t.mul(-r).exp()); }, mu, sigma);

std::cout << "discounted drift " << d.drift << "\n";
```

### Output

```text
discounted drift pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 3.1241143]
```

Example 6. Any number of states, with the time passed through the options. The
function receives the states in a vector, preceded by the time.

```cpp
auto x = pluss::make_series({1.0});
auto mu = pluss::make_series({0.1});
auto sigma = pluss::make_series({{0.2, 0.0}});
std::vector<pluss::Series::ItoState> others = {
    {pluss::make_series({2.0}), pluss::make_series({0.05}), pluss::make_series({{0.0, 0.3}})},
    {pluss::make_series({3.0}), pluss::make_series({0.02}), pluss::make_series({{0.1, 0.1}})}};

pluss::Series::ItoOptions options;
options.time = pluss::make_series(0.5);

// g(t, x, y, z) = t x y z
auto d = x.calc.diff.ito(
    [](const std::vector<pluss::Series>& v) { return v[0].mul(v[1]).mul(v[2]).mul(v[3]); }, mu, sigma, others,
    nullptr, options);

std::cout << "drift     " << d.drift << "\n";
std::cout << "diffusion " << d.diffusion << "\n";
```

### Output

```text
drift     pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[6.43]
diffusion pluss::Series(shape=[1,2], strides=[2,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.7, 0.55]]
```

Example 7. Raw and compensated jumps. Two atoms, jumps of `+0.4` at rate 2 and
`−0.7` at rate 1. The raw form leaves the drift alone and reports the effect of each
jump; the compensated form adds the integral of `g(x + z) − g(x) − g'(x) z`.

```cpp
auto x = pluss::make_series({1.5});
auto mu = pluss::make_series({0.1});
auto sigma = pluss::make_series({0.3});
auto square = [](const pluss::Series& s) { return s.mul(s); };

pluss::Series::JumpMeasure jumps;
jumps.sizes = {pluss::make_series({0.4, -0.7})};
jumps.intensity = pluss::make_series({2.0, 1.0});

jumps.compensated = false;
auto raw = x.calc.diff.ito(square, mu, sigma, jumps);
jumps.compensated = true;
auto compensated = x.calc.diff.ito(square, mu, sigma, jumps);

std::cout << "raw drift         " << raw.drift << "\n";
std::cout << "jump sizes        " << raw.jump_sizes << "\n";
std::cout << "compensated drift " << compensated.drift << "\n";
std::cout << "check: 0.19 + 2*0.16 + 1*0.49 = " << 2 * 1.5 * 0.1 + 0.09 + 2 * 0.16 + 0.49 << "\n";
```

### Output

```text
raw drift         pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.39]
jump sizes        pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1.36],
  [-1.61]]
compensated drift pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.2]
check: 0.19 + 2*0.16 + 1*0.49 = 1.2
```

## Gotchas And Failure Modes

- **`g` must be elementwise.** A function that mixes elements of the receiver (a
  sum, a cumulative sum, a matrix product) is not an independent copy of one
  state per element. The second-order term is read off the diagonal of the
  Hessian, so cross-element dependence is silently dropped rather than detected.
  Reduce after the call, not inside `g`.
- **The same factors for every state.** The covariation term is the dot product of
  volatility rows, which only means something when the rows refer to the same
  Brownian motions. A mismatched trailing length is rejected.
- **The result is a differential, not a path.** `ito` returns the coefficients of
  `dt` and `dW` at a point. Simulating the process, or integrating the
  differential along a path, is a separate step (see
  [`antidiff.ito`](antidiff_ito.md)).
- **Raw and compensated jump parts are different processes.** Choosing the wrong
  one shifts the drift by the mean jump contribution.

Example 8. Misuse is rejected with a message naming the problem. A volatility whose
shape does not match the receiver, in this case:

```cpp
auto x = pluss::make_series({1.0, 2.0});
auto mu = pluss::make_series({0.1, 0.2});
auto sigma = pluss::make_series({0.3, 0.4, 0.5});

try {
  auto d = x.calc.diff.ito([](const pluss::Series& s) { return s.mul(s); }, mu, sigma);
  std::cout << d.drift << "\n";
} catch (const std::exception& e) {
  std::string message = e.what();
  std::cout << "error: " << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
error: Series.calc.diff.ito: sigma must have the shape of the state, or the shape of the state followed by one axis of Brownian factors
```

## Summary

`diff.ito` is Itô's lemma for any smooth function of one or several Itô processes,
with Brownian and jump parts, computed by automatic differentiation. Give it `g`
and the coefficients; it returns the coefficients of `g(X)`.

## See Also

- [`diff.strat`](diff_strat.md): the same differential in the Stratonovich convention.
- [`diff.marcus`](diff_marcus.md): the differential when jumps follow a flow.
- [`antidiff.ito`](antidiff_ito.md): stochastic integrals along a path.
- [`diff`](diff.md): ordinary derivatives of a callable.
