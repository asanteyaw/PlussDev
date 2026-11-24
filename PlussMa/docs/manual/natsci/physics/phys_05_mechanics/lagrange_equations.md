# Lagrange's equations

[Back to Physics 5: Mechanics](../phys_05_mechanics.md)

The same motion can be derived from a different function, the Lagrangian, which is written in terms of positions and velocities instead of positions and momenta. Lagrange's equations turn it into accelerations. This page evaluates a Lagrangian and the quantities derived from it, momentum, effective mass and acceleration, for a pendulum, for a bead whose mass depends on its position, and for a particle in a plane.

## How it works

For a Lagrangian {math}`L(q, v, t)` the canonical momentum is {math}`p = \partial L / \partial v`, the effective mass matrix is {math}`M = \partial^2 L / \partial v^2`, and the motion obeys the Euler-Lagrange equation

```{math}
\frac{\mathrm{d}}{\mathrm{d}t}\frac{\partial L}{\partial v} = \frac{\partial L}{\partial q} \quad\Longrightarrow\quad a = M^{-1}\left(\frac{\partial L}{\partial q} - \frac{\partial^2 L}{\partial v\,\partial q}\,v - \frac{\partial^2 L}{\partial v\,\partial t}\right)
```

A state is made with `mechanics::make_tangent(coordinate, velocity)`, and `mechanics::lagrange(L, state)` returns the `value` of the Lagrangian, the `momentum`, the `mass` matrix, and the acceleration as `flow.velocity`, all found by automatic differentiation.

## A bead on a wire

The first program uses a bead on a wire with {math}`L = (1 + q^2)v^2/2 - q^2/2`, whose effective mass {math}`1 + q^2` grows with position; the acceleration is {math}`a = -q(1 + v^2)/(1 + q^2)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Velocity = mechanics::CoordinateRate<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }  // one coordinate per state
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a bead on a wire whose effective mass grows with position: L = (1 + q^2) v^2 / 2 - q^2 / 2
  const auto bead = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Velocity> &v, const common::SeriesQuantity<common::Time> &) {
    const auto mass = q.si_value().square().add(1.0);
    return common::SeriesQuantity<common::Energy>::from_si(mass.mul(v.si_value().square()).mul(0.5).sub(q.si_value().square().mul(0.5)).sum(-1, false));
  };
  const auto state = mechanics::make_tangent(common::SeriesQuantity<Coordinate>::from_si(col({0.0, 1.0, 2.0})),
                                             common::SeriesQuantity<Velocity>::from_si(col({1.0, 1.0, 1.0})));
  const auto result = mechanics::lagrange(bead, state);
  out.writeln("position q              = ", flat(state.coordinate().si_value()));
  out.writeln("Lagrangian L            = ", flat(result.value.si_value()));
  out.writeln("momentum p = dL/dv      = ", flat(result.momentum.si_value()));
  out.writeln("mass d2L/dv2 = 1 + q^2  = ", flat(result.mass.si_value()));
  out.writeln("acceleration (Euler-Lagrange) = ", flat(result.flow.velocity.si_value()));
}
```

Output

```text
position q              = [0.0000, 1.0000, 2.0000]
Lagrangian L            = [0.5000, 0.5000, 0.5000]
momentum p = dL/dv      = [1.0000, 2.0000, 5.0000]
mass d2L/dv2 = 1 + q^2  = [1.0000, 2.0000, 5.0000]
acceleration (Euler-Lagrange) = [0.0000, -1.0000, -0.8000]
```

The momentum equals the mass times the velocity, `1.0000`, `2.0000` and `5.0000` at {math}`q = 0, 1, 2` for unit speed, which is the mass {math}`1 + q^2`; the accelerations `0.0000`, `-1.0000` and `-0.8000` are the formula {math}`-q(1 + v^2)/(1 + q^2)` evaluated at those points.

## A pendulum

For the pendulum {math}`L = v^2/2 + \cos q` the momentum is the velocity and the acceleration is {math}`-\sin q`. The program evaluates four angles with the same speed and prints the acceleration next to the analytic value.

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
  // a pendulum written as a Lagrangian, L = v^2 / 2 + cos q: the Euler-Lagrange acceleration is -sin q
  const auto pendulum = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Velocity> &v, const common::SeriesQuantity<common::Time> &) {
    return common::SeriesQuantity<common::Energy>::from_si(v.si_value().square().mul(0.5).add(q.si_value().cos()).sum(-1, false));
  };
  const auto angles = std::vector<double>{0.2, 1.0, 2.0, 3.0};
  const auto state = mechanics::make_tangent(common::SeriesQuantity<Coordinate>::from_si(col(angles)), common::SeriesQuantity<Velocity>::from_si(col({0.7, 0.7, 0.7, 0.7})));
  const auto result = mechanics::lagrange(pendulum, state);
  std::vector<double> analytic;
  for (double q : angles) analytic.push_back(-std::sin(q));
  out.writeln("angle q                 = ", angles);
  out.writeln("momentum p = dL/dv      = ", flat(result.momentum.si_value()));
  out.writeln("acceleration            = ", flat(result.flow.velocity.si_value()));
  out.writeln("analytic -sin q         = ", analytic);
}
```

Output

```text
angle q                 = [0.20000, 1.00000, 2.00000, 3.00000]
momentum p = dL/dv      = [0.70000, 0.70000, 0.70000, 0.70000]
acceleration            = [-0.19867, -0.84147, -0.90930, -0.14112]
analytic -sin q         = [-0.19867, -0.84147, -0.90930, -0.14112]
```

The accelerations `-0.19867`, `-0.84147`, `-0.90930` and `-0.14112` match {math}`-\sin q` and the momentum stays at the velocity `0.70000`.

## Two degrees of freedom

With two coordinates the mass is a {math}`2 \times 2` matrix and every quantity gains an axis. The program uses a particle in a plane held by a spring with stiffness {math}`1` along {math}`x` and {math}`4` along {math}`y`, for two states, and prints each result as a flat list in state order.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two coordinates: a particle in a plane held by a spring, L = (vx^2 + vy^2) / 2 - (x^2 + 4 y^2) / 2
  const auto plane = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Velocity> &v, const common::SeriesQuantity<common::Time> &) {
    const auto x = q.si_value().select(-1, 0), y = q.si_value().select(-1, 1);
    return common::SeriesQuantity<common::Energy>::from_si(v.si_value().square().sum(-1, false).mul(0.5).sub(x.square().add(y.square().mul(4.0)).mul(0.5)));
  };
  const auto state = mechanics::make_tangent(common::SeriesQuantity<Coordinate>::from_si(pluss::series::make({{1.0, 0.5}, {2.0, -1.0}})),
                                             common::SeriesQuantity<Velocity>::from_si(pluss::series::make({{0.0, 1.0}, {1.0, 1.0}})));
  const auto result = mechanics::lagrange(plane, state);
  out.writeln("Lagrangian for two states   = ", flat(result.value.si_value()));
  out.writeln("momenta (px, py per state)  = ", flat(result.momentum.si_value()));
  out.writeln("mass matrices (2x2 per state) = ", flat(result.mass.si_value()));
  out.writeln("accelerations (ax, ay)      = ", flat(result.flow.velocity.si_value()));
}
```

Output

```text
Lagrangian for two states   = [-0.5000, -3.0000]
momenta (px, py per state)  = [0.0000, 1.0000, 1.0000, 1.0000]
mass matrices (2x2 per state) = [1.0000, 0.0000, 0.0000, 1.0000, 1.0000, 0.0000, 0.0000, 1.0000]
accelerations (ax, ay)      = [-1.0000, -2.0000, -2.0000, 4.0000]
```

The mass matrices are the identity for both states, the accelerations {math}`(-x, -4y)` are `(-1.0000, -2.0000)` and `(-2.0000, 4.0000)`, and the Lagrangians are `-0.5000` and `-3.0000`.

## Decisions

- Use `lagrange` when your energies are most natural in velocities; it returns the momentum and the mass matrix with the acceleration so you can convert to the Hamiltonian form.
- Use the `mass` matrix to check that it is positive definite before you trust the acceleration; a Lagrangian whose mass is singular has no acceleration.
- Give every degree of freedom its own entry in the last axis, and batch independent states on the leading axes.

## Pitfalls

The second slot of a tangent state is the velocity `dq/dt`, not the momentum. The program evaluates the bead at `q = 2` with unit speed, then feeds the resulting momentum back in as the velocity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace mechanics = pluss::ma::natsci::physics::mechanics;
namespace common = pluss::ma::natsci::common;

using Coordinate = common::Length;
using Velocity = mechanics::CoordinateRate<Coordinate>;

static pluss::Series col(std::vector<double> v) { const auto n = static_cast<std::int64_t>(v.size()); return pluss::series::make(v).to(pluss::ScalarType::Float64).reshape({n, 1}); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the second slot of a tangent state is the velocity dq/dt. The momentum of a bead whose mass is 1 + q^2 is a different number, and feeding it in as if it were a velocity changes the acceleration
  const auto bead = [](const common::SeriesQuantity<Coordinate> &q, const common::SeriesQuantity<Velocity> &v, const common::SeriesQuantity<common::Time> &) {
    const auto mass = q.si_value().square().add(1.0);
    return common::SeriesQuantity<common::Energy>::from_si(mass.mul(v.si_value().square()).mul(0.5).sub(q.si_value().square().mul(0.5)).sum(-1, false));
  };
  const auto q = common::SeriesQuantity<Coordinate>::from_si(col({2.0}));
  const auto moving = mechanics::make_tangent(q, common::SeriesQuantity<Velocity>::from_si(col({1.0})));
  const auto result = mechanics::lagrange(bead, moving);
  out.writeln("velocity 1: momentum ", flat(result.momentum.si_value()), ", acceleration ", flat(result.flow.velocity.si_value()));
  const double p = flat(result.momentum.si_value())[0];
  const auto mistaken = mechanics::make_tangent(q, common::SeriesQuantity<Velocity>::from_si(col({p})));
  out.writeln("momentum ", p, " used as the velocity: acceleration ", flat(mechanics::lagrange(bead, mistaken).flow.velocity.si_value()));
}
```

Output

```text
velocity 1: momentum [5.0000], acceleration [-0.8000]
momentum 5.0000 used as the velocity: acceleration [-10.4000]
```

At `q = 2` the momentum of the unit-speed bead is `5.0000` (the mass is 5) and the acceleration `-0.8000`. Using that momentum as the velocity gives `-10.4000`, thirteen times larger. When the mass depends on position, momentum and velocity are no longer the same number.

## Where next

[Hamilton's equations](hamiltons_equations.md) is the momentum form of the same motion. [Symplectic integration](symplectic_integration.md) steps it forward in time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="symplectic_integration.html">
    <span class="label">Previous</span>
    <span class="title">Symplectic integration</span>
  </a>
</div>
<!-- apiref:nav:end -->
