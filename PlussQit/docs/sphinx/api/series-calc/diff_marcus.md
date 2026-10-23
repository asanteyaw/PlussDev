<!-- pluss-hand-authored -->

# Series::CalcAccessor::diff.marcus

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signatures

```cpp
Series::ItoDifferential Series::CalcAccessor::DiffAccessor::marcus(
    const std::function<Series(const Series&)>& g,
    const Series& mu, const Series& sigma,
    const std::function<Series(const Series&)>& h,
    const Series::JumpMeasure& jumps) const

Series::ItoDifferential marcus(g, mu, sigma, h, jumps,
    const Series::ItoOptions& options) const

Series marcus_flow(const std::function<Series(const Series&)>& h,
    const Series& z, std::int64_t steps = 32) const
```

## What It Computes

The Marcus (canonical) differential describes a process with a diffusion part and
jumps whose effect depends on the state:

```text
dX = μ dt + σ ∘ dW + h(X−) ◇ dJ
```

A jump `z` of the driver `J` does not simply add `z`. It moves the state along the
flow of the equation `dy/du = h(y) z` for `u` from 0 to 1, a limit of ever-faster
smooth motion (Marcus 1978, 1981; Kurtz, Pardoux and Protter 1995; Chechkin and
Pavlyukevich 2014). Two things follow:

- The ordinary chain rule holds across jumps: `g(X)` jumps by exactly `g(φ_z(X−))
  − g(X−)`, where `φ_z` is the time-one flow.
- The continuous part is Stratonovich: drift `g'(x) μ`, diffusion `g'(x) σ`.

With `jumps.compensated = true` the jump part is a martingale, and the drift gains
`Σ ν_k [g(φ_k(x)) − g(x) − g'(x) h(x) z_k]`.

`marcus_flow` returns the time-one flow itself: where a jump of size `z` leaves the
state. It is followed by a fourth-order Runge–Kutta scheme with `steps` steps per
jump (32 by default), so a large jump may need more steps for full accuracy.

## Parameters

- `g`, `mu`, `sigma`: as for [`diff.strat`](diff_strat.md). `mu` and `sigma` are in
  Stratonovich form.
- `h` is the jump coefficient, a function of the state. The special cases are `h = 1`
  (additive), `h = x` (multiplicative: `log x` moves by exactly `z`), and
  `h = x(1 − x)` (logistic: the state stays inside (0, 1)).
- `jumps` is an atomic measure as in [`diff.ito`](diff_ito.md). `sizes[0]` holds the
  jump sizes `z_k` of the scalar driver.
- `options.flow_steps` sets the Runge–Kutta steps per jump.
- `z` (for `marcus_flow`) is broadcast against the receiver: one jump size per point.
- `steps` (for `marcus_flow`) is the number of Runge–Kutta steps.

## Examples

Example 1. The flow of three jump coefficients, each with a closed form: `y + z`,
`y e^z`, and `y / (y + (1 − y) e^(−z))`.

```cpp
auto y = pluss::make_series({0.2, 0.5, 0.9});
auto z = pluss::make_series({1.5, -2.0, 3.0});

auto additive = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(0.0).add(1.0); }, z);
auto multiplicative = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s; }, z);
auto logistic = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(s.neg().add(1.0)); }, z);

std::cout << "additive       " << additive << "\n";
std::cout << "multiplicative " << multiplicative << "\n";
std::cout << "logistic       " << logistic << "\n";
```

### Output

```text
additive       pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.7, -1.5, 3.9]
multiplicative pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.89633776, 0.06766766, 18.076951]
logistic       pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.52839582, 0.11920293, 0.99449853]
```

Example 2. More Runge–Kutta steps tighten a large jump. The exact multiplicative
flow from 0.9 with `z = 3` is `0.9 e³ = 18.076983`.

```cpp
auto y = pluss::make_series({0.9});
auto z = pluss::make_series({3.0});
auto h = [](const pluss::Series& s) { return s; };

std::cout << std::setprecision(10);
std::cout << "8 steps    " << y.calc.diff.marcus_flow(h, z, 8).item<double>() << "\n";
std::cout << "32 steps   " << y.calc.diff.marcus_flow(h, z, 32).item<double>() << "\n";
std::cout << "256 steps  " << y.calc.diff.marcus_flow(h, z, 256).item<double>() << "\n";
std::cout << "exact      " << 0.9 * std::exp(3.0) << "\n";
```

### Output

```text
8 steps    18.07043651
32 steps   18.07695094
256 steps  18.07698322
exact      18.07698323
```

Example 3. The differential for a multiplicative jump. For `g = log`, the jump sizes
of `g(X)` are exactly the atoms, and the continuous part is Stratonovich.

```cpp
auto x = pluss::make_series({1.5});
auto mu = x.mul(0.1);
auto sigma = x.mul(0.3);

pluss::Series::JumpMeasure jumps;
jumps.sizes = {pluss::make_series({0.4, -0.7})};
jumps.intensity = pluss::make_series({2.0, 1.0});

auto d = x.calc.diff.marcus([](const pluss::Series& s) { return s.log(); }, mu, sigma,
                            [](const pluss::Series& s) { return s; }, jumps);

std::cout << "drift     " << d.drift << "\n";
std::cout << "diffusion " << d.diffusion << "\n";
std::cout << "jump sizes " << d.jump_sizes << "\n";
```

### Output

```text
drift     pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.1]
diffusion pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0.3]]
jump sizes pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[0.4],
  [-0.7]]
```

Example 4. A compensated jump part for the same process adds the integral of
`g(φ(x)) − g(x) − g'(x) h(x) z`. For `g = log` and `h = x` the bracket is zero
(`log` moves by exactly `z`, and `g'(x) h(x) z = z`), so the drift is unchanged.

```cpp
auto x = pluss::make_series({1.5});
auto mu = x.mul(0.1);
auto sigma = x.mul(0.3);

pluss::Series::JumpMeasure jumps;
jumps.sizes = {pluss::make_series({0.4, -0.7})};
jumps.intensity = pluss::make_series({2.0, 1.0});
jumps.compensated = true;

auto d = x.calc.diff.marcus([](const pluss::Series& s) { return s.log(); }, mu, sigma,
                            [](const pluss::Series& s) { return s; }, jumps);

std::cout << "compensated drift " << d.drift << "\n";
```

### Output

```text
compensated drift pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[0.1]
```

## Gotchas And Failure Modes

- Only a scalar driver is supported: `jumps.sizes` holds one entry.
- The drift and volatility must already be in Stratonovich form.
- The flow is approximate. For jumps that are large relative to the scale of `h`,
  raise `steps` or `options.flow_steps` and check the result against a case with a
  known flow.
- A full Marcus *simulation* of the process (drawing jumps inside a solver) is not
  provided; this page gives the differential and the flow.

## Summary

`diff.marcus` is the chain rule for processes whose jumps follow a flow;
`marcus_flow` is that flow.

## See Also

- [`diff.strat`](diff_strat.md): the continuous part.
- [`antidiff.marcus`](antidiff_marcus.md): the Marcus integral along a path.
