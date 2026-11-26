# Hamilton's equations

[Back to Physics 5: Mechanics](../phys_05_mechanics.md)

Mechanics can be written in terms of one function, the energy of the system expressed through its positions and momenta. Hamilton's equations then give the motion from the derivatives of that function. This page evaluates the Hamiltonian and its flow, checks the automatic derivatives against the analytic ones for a pendulum, and shows the errors raised for states and energies whose shapes do not agree.

## How it works

For a system with coordinates {math}`q` and momenta {math}`p` and a Hamiltonian {math}`H(q, p, t)`, the motion obeys

```{math}
\begin{aligned}
&\frac{\mathrm{d}q}{\mathrm{d}t} = \frac{\partial H}{\partial p}, \\
&\frac{\mathrm{d}p}{\mathrm{d}t} = -\frac{\partial H}{\partial q}
\end{aligned}
```

A state is made with `mechanics::make_phase(coordinate, momentum)` from two quantities of the same shape, in which the last axis holds the degrees of freedom and any leading axes are independent states, so many starting points move together. `mechanics::hamiltonian(H, state)` evaluates the energy function you supply, a callback of `(q, p, t)` that returns one energy per state, and returns the `value` and the `flow`, whose two parts `flow.coordinate` and `flow.momentum` are the two derivatives above, obtained by automatic differentiation of the callback.

## A spring

The first program uses a unit mass on a spring, {math}`H = (q^2 + p^2)/2`, for two states.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace mechanics = natsci::physics::mechanics;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }  // one coordinate per state
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a unit mass on a spring, H = (q^2 + p^2) / 2, for two starting points at once
  const auto spring = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().square().add(p.si_value().square()).mul(0.5).sum(-1, false));
  };
  const auto state = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({1.0, 3.0})),
                                           common::SeriesQuantity<Momentum>::from_si(col({0.5, -2.0})));
  const auto h = mechanics::hamiltonian(spring, state);
  out.writeln("position q                = ", flat(state.coordinate().si_value()));
  out.writeln("momentum p                = ", flat(state.momentum().si_value()));
  out.writeln("energy H                  = ", flat(h.value.si_value()));
  out.writeln("dq/dt = dH/dp             = ", flat(h.flow.coordinate.si_value()));
  out.writeln("dp/dt = -dH/dq            = ", flat(h.flow.momentum.si_value()));
}
```

Output

```text
position q                = [1.0000, 3.0000]
momentum p                = [0.5000, -2.0000]
energy H                  = [0.6250, 6.5000]
dq/dt = dH/dp             = [0.5000, -2.0000]
dp/dt = -dH/dq            = [-1.0000, -3.0000]
```

For the spring the energies are `0.6250` and `6.5000`, and the flow reproduces the equations: {math}`\mathrm{d}q/\mathrm{d}t` equals the momentum (`0.5000`, `-2.0000`) and {math}`\mathrm{d}p/\mathrm{d}t` equals minus the position (`-1.0000`, `-3.0000`).

## A pendulum

For the pendulum {math}`H = p^2/2 + (1 - \cos q)` the second equation is {math}`\mathrm{d}p/\mathrm{d}t = -\sin q`. The program evaluates the flow at four angles, with the momentum at zero, and prints it next to the analytic value.

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
  // a pendulum, H = p^2 / 2 + (1 - cos q): compare the automatic flow with the analytic one, dp/dt = -sin q
  const auto energy = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).add(q.si_value().cos().neg().add(1.0)).sum(-1, false));
  };
  const auto angles = std::vector<double>{0.2, 1.0, 2.0, 3.0};
  const auto state = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col(angles)), common::SeriesQuantity<Momentum>::from_si(col({0.0, 0.0, 0.0, 0.0})));
  const auto h = mechanics::hamiltonian(energy, state);
  std::vector<double> analytic;
  for (double q : angles) analytic.push_back(-std::sin(q));
  out.writeln("angle q                    = ", angles);
  out.writeln("dp/dt from the Hamiltonian = ", flat(h.flow.momentum.si_value()));
  out.writeln("analytic -sin q            = ", analytic);
}
```

Output

```text
angle q                    = [0.20000, 1.00000, 2.00000, 3.00000]
dp/dt from the Hamiltonian = [-0.19867, -0.84147, -0.90930, -0.14112]
analytic -sin q            = [-0.19867, -0.84147, -0.90930, -0.14112]
```

The derivative found automatically is `-0.19867`, `-0.84147`, `-0.90930` and `-0.14112` at the four angles, identical to {math}`-\sin q` to the five digits printed.

## Inputs that do not fit

A state whose momentum does not have the shape of its coordinate is rejected when it is built, and an energy callback that returns the wrong number of values is rejected when it is evaluated. Both messages name the function and the shape rule.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

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
  // a state needs one momentum for every coordinate; a mismatch is rejected when the state is built
  try {
    (void)mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({1.0, 2.0})), common::SeriesQuantity<Momentum>::from_si(pluss::series::make({0.5, -1.0, 2.0})));
  } catch (const pluss::Error &e) {
    out.writeln("mismatched shapes: ", e.message());
  }
  // a Hamiltonian that returns the wrong number of values is rejected when it is evaluated
  const auto wrong = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Momentum> &, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(q.si_value().square().mul(0.5));   // one value per coordinate, not per state
  };
  const auto state = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({1.0, 2.0})), common::SeriesQuantity<Momentum>::from_si(col({0.5, -1.0})));
  try {
    (void)mechanics::hamiltonian(wrong, state);
  } catch (const pluss::Error &e) {
    out.writeln("wrong energy layout: ", e.message());
  }
}
```

Output

```text
mismatched shapes:  natsci::mechanics::make_phase: momentum shape must match coordinate shape
wrong energy layout:  natsci::mechanics::hamiltonian: callback output must be scalar or have the state batch shape
```

The two error lines are the messages raised: a momentum shape that must match the coordinate shape, and a callback output that must be a scalar or have the state's batch shape.

## Decisions

- Write the Hamiltonian in terms of the canonical momentum, `H(q, p, t)`; if your energy is written with velocity, use the [Lagrange page](lagrange_equations.md) instead.
- Return one energy per state from the callback, summed over the last axis, so a batch of states stays a batch.
- Use `symplectic_step` rather than a general integrator when you go on to follow the flow in time.

## Pitfalls

The relation `dq/dt = p` holds only for a unit mass. The program takes a free particle of mass 2 with momentum 1, and the same energy written without the mass.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace mechanics = natsci::physics::mechanics;

using Coordinate = common::Length;
using Momentum = mechanics::CanonicalMomentum<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a free particle of mass m = 2 with momentum p = 1: the velocity dq/dt is p / m = 0.5, not p
  const double mass = 2.0;
  const auto free_particle = [mass](const common::SeriesQuantity<Coordinate> &, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5 / mass).sum(-1, false));
  };
  const auto state = mechanics::make_phase(common::SeriesQuantity<Coordinate>::from_si(col({0.0})), common::SeriesQuantity<Momentum>::from_si(col({1.0})));
  const auto h = mechanics::hamiltonian(free_particle, state);
  out.writeln("momentum p = ", flat(state.momentum().si_value()), ", dq/dt = ", flat(h.flow.coordinate.si_value()), ", energy = ", flat(h.value.si_value()));
  // the same energy written without the mass, H = p^2 / 2, is the unit-mass particle: dq/dt equals p
  const auto unit = [](const common::SeriesQuantity<Coordinate> &, const common::SeriesQuantity<Momentum> &p, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(p.si_value().square().mul(0.5).sum(-1, false));
  };
  out.writeln("H = p^2 / 2 (forgot the mass): dq/dt = ", flat(mechanics::hamiltonian(unit, state).flow.coordinate.si_value()));
}
```

Output

```text
momentum p = [1.0000], dq/dt = [0.5000], energy = [0.2500]
H = p^2 / 2 (forgot the mass): dq/dt = [1.0000]
```

With the mass in the Hamiltonian, `H = p^2 / (2 m)`, the velocity is `0.5000` and the energy `0.2500`. If you leave the mass out, `H = p^2 / 2`, the velocity is `1.0000`: the program quietly solves a different particle. Put every constant that appears in the energy into the callback.

## Where next

[Lagrange's equations](lagrange_equations.md) gives the same motion from positions and velocities. [Symplectic integration](symplectic_integration.md) follows the flow in time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="symplectic_integration.html">
    <span class="label">Next</span>
    <span class="title">Symplectic integration</span>
  </a>
</div>
<!-- apiref:nav:end -->
