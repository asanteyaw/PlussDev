<!-- generated from dynamics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robot dynamics

This branch relates joint motion to joint forces. `torques` gives the torques of a motion, `inertia` and `bias` its parts, `accel` the accelerations under torques, and `ke` and `pe` the energies. `DynamicsOptions` sets gravity.

## Choosing A Surface

Use `torques` to find the torques that produce a planned motion, `accel` to find the motion that torques produce, and `inertia` with `bias` when the equations of motion are needed in parts, for example to design a controller.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Torques of a planar arm.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const pluss::Series q = vec({0.4, -0.8}), qd = vec({1.3, -0.7}), qdd = vec({0.9, -1.4});
  const pluss::Series tau = rb::torques(robot, q, qd, qdd, in_plane());
  out.writeln("joint torques", flat(tau));
  // the same torques from the parts: M qdd + bias
  const pluss::Series parts = rb::inertia(robot, q).matmul(qdd.unsqueeze(-1)).squeeze(-1).add(rb::bias(robot, q, qd, in_plane()));
  out.writeln("M qdd + bias   ", flat(parts));
  // holding still: only gravity
  out.writeln("holding the pose", flat(rb::torques(robot, q, vec({0, 0}), vec({0, 0}), in_plane())));
}
```

Output

```text
joint torques [15.24292, 3.44218]
M qdd + bias    [15.24292, 3.44218]
holding the pose [14.90875, 3.61424]
```
### Example 2

The inertia matrix.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(arm_links());
  const double q2 = -0.8;
  const pluss::Series m = rb::inertia(robot, vec({0.4, q2}));
  out.writeln("mass matrix", flat(m));
  // closed form of the planar two-link inertia matrix
  const double c2 = std::cos(q2);
  const double a = 1.0 * 0.25 * 0.25 + 2.0 * (0.5 * 0.5 + 0.2 * 0.2 + 2 * 0.5 * 0.2 * c2) + 0.02 + 0.03;
  const double b = 2.0 * (0.2 * 0.2 + 0.5 * 0.2 * c2) + 0.03;
  out.writeln("closed form ", a, b, b, 2.0 * 0.2 * 0.2 + 0.03);
  out.writeln("symmetric", m.sub(m.mT()).abs().amax().item<double>() < 1e-12);
  // the inertia seen at the elbow depends on the elbow angle, not on the shoulder angle
  out.writeln("mass matrix at another shoulder angle", flat(rb::inertia(robot, vec({1.4, q2}))));
}
```

Output

```text
mass matrix [0.97118, 0.24934, 0.24934, 0.11000]
closed form  0.97118 0.24934 0.24934 0.11000
symmetric true
mass matrix at another shoulder angle [0.97118, 0.24934, 0.24934, 0.11000]
```
### Example 3

A simulation that conserves energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// planar two-link arm in the x-y plane: both joints about z, link lengths 0.5 and 0.4 m, masses 1 and 2 kg
static rb::Links arm_links() {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat({0.25, 0, 0, 0.2, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}
static rb::DynamicsOptions in_plane() {
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});   // gravity along -y: the arm moves in the x-y plane
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = rb::make(arm_links());
  const auto energy = [&](const pluss::Series &q, const pluss::Series &qd) {
    return rb::ke(robot, q, qd).item<double>() + rb::pe(robot, q, in_plane()).item<double>();
  };
  const auto accel = [&](const pluss::Series &q, const pluss::Series &qd) {
    return rb::accel(robot, q, qd, vec({0, 0}), in_plane());
  };
  // the arm is released from rest and swings for one second (classical fourth-order Runge-Kutta, 1 ms steps)
  pluss::Series q = vec({0.3, 0.5}), v = vec({0.0, 0.0});
  const double e0 = energy(q, v);
  const double h = 1e-3;
  out.writeln("  t     q1       q2     total energy");
  for (int k = 0; k <= 1000; ++k) {
    if (k % 250 == 0) out.writeln(k * h, flat(q)[0], flat(q)[1], energy(q, v));
    const pluss::Series k1x = v, k1v = accel(q, v);
    const pluss::Series k2x = v.add(k1v.mul(h / 2)), k2v = accel(q.add(k1x.mul(h / 2)), k2x);
    const pluss::Series k3x = v.add(k2v.mul(h / 2)), k3v = accel(q.add(k2x.mul(h / 2)), k3x);
    const pluss::Series k4x = v.add(k3v.mul(h)), k4v = accel(q.add(k3x.mul(h)), k4x);
    q = q.add(k1x.add(k2x.mul(2)).add(k3x.mul(2)).add(k4x).mul(h / 6));
    v = v.add(k1v.add(k2v.mul(2)).add(k3v.mul(2)).add(k4v).mul(h / 6));
  }
  out.writeln("energy drift over the second below 1e-8", std::abs(energy(q, v) - e0) < 1e-8);
}
```

Output

```text
  t     q1       q2     total energy
0.000000 0.300000 0.500000 6.438722
0.250000 -0.358344 1.167095 6.438722
0.500000 -1.163493 -0.880443 6.438722
0.750000 -3.107489 0.978707 6.438722
1.000000 -3.126743 -1.204741 6.438722
energy drift over the second below 1e-8 true
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dynamics/DynamicsOptions
dynamics/torques
dynamics/inertia
dynamics/bias
dynamics/accel
dynamics/ke
dynamics/pe
```
<!-- apiref:toctree:end -->
