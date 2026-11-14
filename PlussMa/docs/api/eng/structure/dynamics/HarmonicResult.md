<!-- generated from HarmonicResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `structure::HarmonicResult`

The steady response at each frequency.

## Signature

```cpp
structure::HarmonicResult
```

## What It Does

The result of `harmonic`. Displacements, velocities, accelerations and reactions are complex amplitudes: the size is the amplitude and the angle is the phase relative to the load.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `omega` | The angular frequencies, `[frequency]`. |  |
| `displacement` | Complex displacement amplitude, `[..., frequency, node, 3]`. |  |
| `velocity` | `i omega` times the displacement. |  |
| `acceleration` | `-omega^2` times the displacement. |  |
| `reaction` | Complex support force and moment, zero at free degrees of freedom. |  |
| `residual_norm` | Out-of-balance force at each frequency. |  |
| `relative_residual` | The residual as a fraction of the load. |  |
| `converged` | One where the solve at that frequency converged. |  |
| `iterations` | GMRES iterations. |  |
| `operator_evaluations` | Applications of the dynamic stiffness. |  |
| `restarts` | GMRES restarts. |  |

## Returns

Returned by `harmonic`.

## Errors

Reading a field does not solve again.

## Interpretation and Use

The physical motion at one frequency is the real part of the amplitude times `e^{i omega t}`; the phase lag behind the load is the negative of the angle of the amplitude.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Amplitudes, shapes and diagnostics over four frequencies.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const std::vector<double> frequencies = {10, 40.5, 100, 253.8};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = 10.0;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3});

  structure::HarmonicOptions stiffness;
  stiffness.stiffness_damping = 1e-4;   // zeta_n = b omega_n / 2, 0.002 at the first mode
  const auto r = structure::harmonic(frame, load, vec(frequencies), wall, stiffness);
  out.writeln("stiffness damping: tip amplitude (mm) ", flat(r.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)), ", converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", reaction ", r.reaction.sizes(), ", relative residual ", flat(r.relative_residual));
  out.writeln("iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts);

  // mass damping: zeta_n = a / (2 omega_n), the same 0.002 at the first mode, but much heavier damping at low frequency and none at high frequency
  const double omega1 = flat(structure::modes(frame, wall).omega)[0];
  structure::HarmonicOptions mass;
  mass.mass_damping = 2.0 * 0.002 * omega1;
  const auto m = structure::harmonic(frame, load, vec(frequencies), wall, mass);
  out.writeln("mass damping:      tip amplitude (mm) ", flat(m.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)));

  // velocity and acceleration
  out.writeln("|velocity| / (omega |u|) at the tip: ", flat(r.velocity.select(-2, members).select(-1, 1).abs().div(r.displacement.select(-2, members).select(-1, 1).abs()).div(vec(frequencies))));

  // undamped: the amplitude grows without bound as the frequency approaches the natural frequency
  const std::vector<std::pair<const char *, double>> offsets = {{"1e-2", 1e-2}, {"1e-3", 1e-3}};
  for (const auto &offset : offsets) {
    const auto u = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1 * (1.0 + offset.second)}), wall);
    out.writeln("no damping, ", offset.first, " above omega_1: tip amplitude (mm) ", 1000.0 * flat(u.displacement.select(-2, members).select(-1, 1).abs())[0]);
  }

  // exactly at the natural frequency the dynamic stiffness is singular: the solve reports that it did not converge
  structure::HarmonicOptions undamped;
  undamped.fail_on_nonconvergence = false;
  const auto exact = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  out.writeln("undamped at omega_1, failure off: converged ", exact.converged.all().item<bool>());
  try {
    undamped.fail_on_nonconvergence = true;
    (void)structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  } catch (const std::exception &e) {
    std::cout << "undamped at omega_1, failure on: " << e.what() << "\n";
  }
}
```

Output

```text
stiffness damping: tip amplitude (mm) [1.7010, 383.4239, 0.2505, 1.5585], converged true
shapes: displacement [4, 9, 3], reaction [4, 9, 3], relative residual [0.0000, 0.0000, 0.0000, 0.0000]
iterations 24, operator evaluations 26, restarts 1
mass damping:      tip amplitude (mm) [1.7010, 388.2302, 0.2505, 55.7885]
|velocity| / (omega |u|) at the tip: [1.0000, 1.0000, 1.0000, 1.0000]
no damping, 1e-2 above omega_1: tip amplitude (mm) 77.2242
no damping, 1e-3 above omega_1: tip amplitude (mm) 776.1488
undamped at omega_1, failure off: converged false
undamped at omega_1, failure on: eng::structure::harmonic: dynamic stiffness solve did not converge; a sampled frequency may be singular or poorly conditioned
```
### Example 2

Complex amplitudes against a mode sum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>
#include <complex>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const double force = 10.0, b = 1e-4;
  const std::vector<double> frequencies = {10, 30, 100, 150};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = force;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  structure::HarmonicOptions options;
  options.stiffness_damping = b;
  const auto h = structure::harmonic(frame, pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3}), vec(frequencies), wall, options);
  const auto direct = h.displacement.select(-2, members).select(-1, 1);
  const auto re = flat(direct.real()), im = flat(direct.imag());

  // the same response as a sum over the first six mass-normalised modes
  structure::EigOptions eig;
  eig.count = 6;
  const auto modes = structure::modes(frame, wall, eig);
  const auto omega = flat(modes.omega), shape = flat(modes.shape);
  out.writeln("omega    direct (mm, phase deg)       mode sum (mm, phase deg)");
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::complex<double> sum = 0.0;
    for (int n = 0; n < 6; ++n) {
      const double phi = shape[n * 3 * (members + 1) + tip];
      sum += phi * phi * force / std::complex<double>(omega[n] * omega[n] - frequencies[f] * frequencies[f], b * omega[n] * omega[n] * frequencies[f]);
    }
    out.writeln(frequencies[f], "   ", 1000.0 * std::hypot(re[f], im[f]), " ", std::atan2(im[f], re[f]) * 180.0 / M_PI, "      ", 1000.0 * std::abs(sum), " ", std::arg(sum) * 180.0 / M_PI);
  }
}
```

Output

```text
omega    direct (mm, phase deg)       mode sum (mm, phase deg)
10.0000   1.7010 -0.0609      1.7007 -0.0609
30.0000   3.4886 -0.3780      3.4883 -0.3780
100.0000   0.2505 -179.7192      0.2508 -179.7201
150.0000   0.0538 -178.2308      0.0541 -178.2438
```
### Example 3

An undamped resonance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <iostream>

namespace structure = pluss::subsp::eng::structure;
using namespace pluss::tils;

static pluss::Series grid(std::vector<double> v, std::int64_t cols) { return pluss::series::make(v).reshape({(std::int64_t)v.size() / cols, cols}); }
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a straight steel beam of length 2 m along x, with density, divided into equal members
static structure::Frame beam(int members, bool with_density = true) {
  std::vector<double> position;
  std::vector<std::int64_t> from, to;
  for (int i = 0; i <= members; ++i) {
    position.push_back(2.0 * i / members);
    position.push_back(0.0);
  }
  for (int i = 0; i < members; ++i) {
    from.push_back(i);
    to.push_back(i + 1);
  }
  structure::Sections sections{vec(std::vector<double>(members, 1e-3)), vec(std::vector<double>(members, 8.333e-8)), vec(std::vector<double>(members, 200e9))};
  if (with_density) sections.density = vec(std::vector<double>(members, 7850.0));
  return structure::frame({grid(position, 2), from, to}, sections);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const int members = 8;
  const auto frame = beam(members);
  const structure::Fix wall{{0, 0, 0}, {0, 1, 2}, {}};
  const std::size_t tip = 3 * members + 1;
  const std::vector<double> frequencies = {10, 40.5, 100, 253.8};
  std::vector<double> loads;
  for (std::size_t f = 0; f < frequencies.size(); ++f) {
    std::vector<double> load(3 * (members + 1), 0.0);
    load[tip] = 10.0;
    loads.insert(loads.end(), load.begin(), load.end());
  }
  const auto load = pluss::series::make(loads).reshape({(std::int64_t)frequencies.size(), members + 1, 3});

  structure::HarmonicOptions stiffness;
  stiffness.stiffness_damping = 1e-4;   // zeta_n = b omega_n / 2, 0.002 at the first mode
  const auto r = structure::harmonic(frame, load, vec(frequencies), wall, stiffness);
  out.writeln("stiffness damping: tip amplitude (mm) ", flat(r.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)), ", converged ", r.converged.all().item<bool>());
  out.writeln("shapes: displacement ", r.displacement.sizes(), ", reaction ", r.reaction.sizes(), ", relative residual ", flat(r.relative_residual));
  out.writeln("iterations ", r.iterations, ", operator evaluations ", r.operator_evaluations, ", restarts ", r.restarts);

  // mass damping: zeta_n = a / (2 omega_n), the same 0.002 at the first mode, but much heavier damping at low frequency and none at high frequency
  const double omega1 = flat(structure::modes(frame, wall).omega)[0];
  structure::HarmonicOptions mass;
  mass.mass_damping = 2.0 * 0.002 * omega1;
  const auto m = structure::harmonic(frame, load, vec(frequencies), wall, mass);
  out.writeln("mass damping:      tip amplitude (mm) ", flat(m.displacement.select(-2, members).select(-1, 1).abs().mul(1000.0)));

  // velocity and acceleration
  out.writeln("|velocity| / (omega |u|) at the tip: ", flat(r.velocity.select(-2, members).select(-1, 1).abs().div(r.displacement.select(-2, members).select(-1, 1).abs()).div(vec(frequencies))));

  // undamped: the amplitude grows without bound as the frequency approaches the natural frequency
  const std::vector<std::pair<const char *, double>> offsets = {{"1e-2", 1e-2}, {"1e-3", 1e-3}};
  for (const auto &offset : offsets) {
    const auto u = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1 * (1.0 + offset.second)}), wall);
    out.writeln("no damping, ", offset.first, " above omega_1: tip amplitude (mm) ", 1000.0 * flat(u.displacement.select(-2, members).select(-1, 1).abs())[0]);
  }

  // exactly at the natural frequency the dynamic stiffness is singular: the solve reports that it did not converge
  structure::HarmonicOptions undamped;
  undamped.fail_on_nonconvergence = false;
  const auto exact = structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  out.writeln("undamped at omega_1, failure off: converged ", exact.converged.all().item<bool>());
  try {
    undamped.fail_on_nonconvergence = true;
    (void)structure::harmonic(frame, load.slice(0, 1, 2), vec({omega1}), wall, undamped);
  } catch (const std::exception &e) {
    std::cout << "undamped at omega_1, failure on: " << e.what() << "\n";
  }
}
```

Output

```text
stiffness damping: tip amplitude (mm) [1.7010, 383.4239, 0.2505, 1.5585], converged true
shapes: displacement [4, 9, 3], reaction [4, 9, 3], relative residual [0.0000, 0.0000, 0.0000, 0.0000]
iterations 24, operator evaluations 26, restarts 1
mass damping:      tip amplitude (mm) [1.7010, 388.2302, 0.2505, 55.7885]
|velocity| / (omega |u|) at the tip: [1.0000, 1.0000, 1.0000, 1.0000]
no damping, 1e-2 above omega_1: tip amplitude (mm) 77.2242
no damping, 1e-3 above omega_1: tip amplitude (mm) 776.1488
undamped at omega_1, failure off: converged false
undamped at omega_1, failure on: eng::structure::harmonic: dynamic stiffness solve did not converge; a sampled frequency may be singular or poorly conditioned
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NonlinearMotionResult.html">
    <span class="label">Previous</span>
    <span class="title">NonlinearMotionResult</span>
  </a>
  <a class="next-link" href="motion.html">
    <span class="label">Next</span>
    <span class="title">motion</span>
  </a>
</div>
<!-- apiref:nav:end -->
