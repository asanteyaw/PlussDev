# Symplectic integration

[Back to Physics 5: Mechanics](../phys_05_mechanics.md)

To follow the motion in time one integrates Hamilton's equations. A general-purpose integrator drifts in energy; a symplectic one, which preserves the geometric structure of Hamiltonian flow, keeps the energy error bounded for as long as the motion is followed. This page advances a pendulum with the leapfrog rule, compares it with the simple forward-Euler step, advances several pendulums with one call, and runs the motion backwards.

## How it works

When the Hamiltonian separates into {math}`H = T(p) + V(q)` the leapfrog rule advances a state by a step {math}`h` with a half step in momentum, a full step in position and another half step in momentum:

```{math}
\begin{aligned}
&p_{1/2} = p_0 - \tfrac{h}{2} V'(q_0), \\
&q_1 = q_0 + h\,T'(p_{1/2}), \\
&p_1 = p_{1/2} - \tfrac{h}{2} V'(q_1)
\end{aligned}
```

`mechanics::symplectic_step(state, step, potential, kinetic)` does this with the force and the velocity taken by automatic differentiation of the two energy callbacks `V(q, t)` and `T(p, t)`; its optional fifth argument is the time at the start of the step. The method is second order, so halving the step should reduce the energy error by about four.

## Leapfrog against Euler

The program follows a pendulum released from {math}`q = 2.5` at rest for {math}`20` time units with two step sizes and compares the energy error with that of the forward-Euler update, which is written out in the program from the flow.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }  // one coordinate per state
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // a pendulum with H = p^2 / 2 + (1 - cos q): the force comes from the derivative of the potential, found automatically
  const auto energy = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).add(q.si_value().cos().neg().add(1.0)).sum(-1, false));
  };
  const auto potential = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().cos().neg().add(1.0).sum(-1, false));
  };
  const auto kinetic = [](const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).sum(-1, false));
  };

  const auto start = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({2.5})),
                                           common::SeriesQuantity<Momentum>::from_si(col({0.0})));
  const double e0 = flat(mechanics::hamiltonian(energy, start).value.si_value())[0];
  out.writeln("starting energy = ", e0);

  // leapfrog (symplectic) against forward Euler, about three swings in 20 time units
  for (double dt : {0.1, 0.05}) {
    auto leapfrog = start, euler = start;
    const auto step = common::ScalarQuantity<common::Time>::from_si(dt);
    const int steps = static_cast<int>(20.0 / dt);
    for (int k = 0; k < steps; ++k) {
      leapfrog = mechanics::symplectic_step(leapfrog, step, potential, kinetic);
      const auto h = mechanics::hamiltonian(energy, euler);
      euler = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(euler.coordinate().si_value().add(h.flow.coordinate.si_value().mul(dt))),
                                    common::SeriesQuantity<Momentum>::from_si(euler.momentum().si_value().add(h.flow.momentum.si_value().mul(dt))));
    }
    const double e_leap = flat(mechanics::hamiltonian(energy, leapfrog).value.si_value())[0];
    const double e_euler = flat(mechanics::hamiltonian(energy, euler).value.si_value())[0];
    out.writeln("step ", dt, ", after time 20: leapfrog energy error = ", e_leap - e0, ", Euler energy error = ", e_euler - e0);
  }

  // a negative step runs the motion backwards and returns to the start
  auto there = mechanics::symplectic_step(start, common::ScalarQuantity<common::Time>::from_si(0.5), potential, kinetic);
  auto back = mechanics::symplectic_step(there, common::ScalarQuantity<common::Time>::from_si(-0.5), potential, kinetic, common::ScalarQuantity<common::Time>::from_si(0.5));
  out.writeln("after +0.5 then -0.5: q = ", flat(back.coordinate().si_value())[0], ", p = ", flat(back.momentum().si_value())[0]);
}
```

Output

```text
starting energy = 1.80114
step 0.10000, after time 20: leapfrog energy error = 0.00015, Euler energy error = 0.83907
step 0.05000, after time 20: leapfrog energy error = 0.00004, Euler energy error = 0.44437
after +0.5 then -0.5: q = 2.50000, p = 0.00000
```

The starting energy is `1.80114`. With the step {math}`0.1` the leapfrog energy error after time {math}`20` is `0.00015` and the Euler error `0.83907`; with {math}`0.05` they are `0.00004` and `0.44437`. Halving the step cuts the leapfrog error by a factor of about four, as a second-order rule should, while the Euler error only halves and is four orders of magnitude larger: forward Euler pumps energy into the pendulum, the leapfrog rule does not.

## Many states in one call

Because a state may hold any number of independent pendulums, one call advances all of them. The program releases three pendulums from rest at three angles and steps them for {math}`200` steps of {math}`0.05`, then prints their angles, momenta and energies.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <exception>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;
using Velocity = mechanics::CoordinateRate<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }  // one coordinate per state
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto energy = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).add(q.si_value().cos().neg().add(1.0)).sum(-1, false));
  };
  const auto potential = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().cos().neg().add(1.0).sum(-1, false));
  };
  const auto kinetic = [](const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).sum(-1, false));
  };
  // three pendulums released from rest at different heights, advanced together by one call per step
  auto state = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({0.5, 1.5, 2.5})), common::SeriesQuantity<Momentum>::from_si(col({0.0, 0.0, 0.0})));
  const auto start_energy = flat(mechanics::hamiltonian(energy, state).value.si_value());
  const auto step = common::ScalarQuantity<common::Time>::from_si(0.05);
  for (int k = 0; k < 200; ++k) state = mechanics::symplectic_step(state, step, potential, kinetic);
  out.writeln("after time 10:  angle    = ", flat(state.coordinate().si_value()));
  out.writeln("                momentum = ", flat(state.momentum().si_value()));
  const auto end_energy = flat(mechanics::hamiltonian(energy, state).value.si_value());
  out.writeln("energy at the start      = ", start_energy);
  out.writeln("energy at time 10        = ", end_energy);
}
```

Output

```text
after time 10:  angle    = [-0.45691, -1.05477, 2.46858]
                momentum = [0.19913, -0.91932, 0.19600]
energy at the start      = [0.12242, 0.92926, 1.80114]
energy at time 10        = [0.12241, 0.92915, 1.80115]
```

The three pendulums advanced together keep their energies, `0.12242`, `0.92926` and `1.80114` to `0.12241`, `0.92915` and `1.80115` at time {math}`10`.

## Running backwards

The leapfrog rule is time-reversible: a step of {math}`-h` undoes a step of {math}`+h` exactly, apart from rounding. The program runs {math}`100` steps forward, then {math}`100` steps with the sign of the step reversed, and prints the state at the start, after the forward run and after the return.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <exception>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;
using Velocity = mechanics::CoordinateRate<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }  // one coordinate per state
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto potential = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().cos().neg().add(1.0).sum(-1, false));
  };
  const auto kinetic = [](const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).sum(-1, false));
  };
  // run forward 100 steps, then flip the sign of the step and run 100 steps back: the leapfrog rule is time-reversible
  const auto start = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({1.2})), common::SeriesQuantity<Momentum>::from_si(col({0.3})));
  auto state = start;
  double t = 0.0;
  for (int k = 0; k < 100; ++k) { state = mechanics::symplectic_step(state, common::ScalarQuantity<common::Time>::from_si(0.05), potential, kinetic, common::ScalarQuantity<common::Time>::from_si(t)); t += 0.05; }
  out.writeln("after 100 steps forward:  q = ", flat(state.coordinate().si_value())[0], ", p = ", flat(state.momentum().si_value())[0]);
  for (int k = 0; k < 100; ++k) { state = mechanics::symplectic_step(state, common::ScalarQuantity<common::Time>::from_si(-0.05), potential, kinetic, common::ScalarQuantity<common::Time>::from_si(t)); t -= 0.05; }
  out.writeln("after 100 steps backward: q = ", flat(state.coordinate().si_value())[0], ", p = ", flat(state.momentum().si_value())[0]);
  out.writeln("started at:               q = ", flat(start.coordinate().si_value())[0], ", p = ", flat(start.momentum().si_value())[0]);
}
```

Output

```text
after 100 steps forward:  q = -0.595696, p = 1.010122
after 100 steps backward: q = 1.200000, p = 0.300000
started at:               q = 1.200000, p = 0.300000
```

After the forward run the state is {math}`q = -0.595696`, {math}`p = 1.010122` and after the backward run it is {math}`q = 1.200000`, {math}`p = 0.300000`, the same as at the start to six digits.

## Decisions

- Use `symplectic_step` whenever the energy separates into a kinetic and a potential part; the energy then stays bounded however long you run.
- Choose the step so that it is well below the shortest period of the motion; halving it reduces the error by four.
- Advance many initial states in one call by giving them leading axes, rather than looping.

## Pitfalls

The leapfrog rule keeps the energy bounded only for steps below the stability limit, which for a small swing is 2 time units. The program follows a small-swing pendulum for 200 steps of four sizes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a small-swing pendulum (period 2 pi = 6.28): the leapfrog rule keeps the energy bounded only while the step is below 2 time units
  const auto energy = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).add(q.si_value().cos().neg().add(1.0)).sum(-1, false));
  };
  const auto potential = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().cos().neg().add(1.0).sum(-1, false));
  };
  const auto kinetic = [](const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).sum(-1, false));
  };
  const auto start = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({0.3})), common::SeriesQuantity<Momentum>::from_si(col({0.0})));
  const double e0 = flat(mechanics::hamiltonian(energy, start).value.si_value())[0];
  for (double dt : {0.1, 1.0, 1.9, 2.1}) {
    auto state = start;
    double worst = 0.0;
    for (int k = 0; k < 200; ++k) {
      state = mechanics::symplectic_step(state, common::ScalarQuantity<common::Time>::from_si(dt), potential, kinetic);
      worst = std::max(worst, std::abs(flat(mechanics::hamiltonian(energy, state).value.si_value())[0] - e0));
    }
    out.writeln("step ", dt, ": largest energy error in 200 steps = ", worst, ", final angle = ", flat(state.coordinate().si_value())[0]);
  }
}
```

Output

```text
step 0.1000: largest energy error in 200 steps = 0.0001, final angle = 0.1505
step 1.0000: largest energy error in 200 steps = 0.0111, final angle = 0.2101
step 1.9000: largest energy error in 200 steps = 0.0400, final angle = -0.0409
step 2.1000: largest energy error in 200 steps = 0.4595, final angle = 0.6855
```

With steps of `0.1`, `1.0` and `1.9` the largest energy error is `0.0001`, `0.0111` and `0.0400`. At `2.1`, past the limit, it is `0.4595`, ten times the pendulum's whole energy. Leapfrog does not warn you, so pick the step from the period of the motion.

## Where next

[Hamilton's equations](hamiltons_equations.md) gives the flow this page integrates. [Kepler orbits](../phys_01_orbits/kepler_orbits.md) solves the two-body problem exactly instead.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hamiltons_equations.html">
    <span class="label">Previous</span>
    <span class="title">Hamilton's equations</span>
  </a>
  <a class="next-link" href="lagrange_equations.html">
    <span class="label">Next</span>
    <span class="title">Lagrange's equations</span>
  </a>
</div>
<!-- apiref:nav:end -->
