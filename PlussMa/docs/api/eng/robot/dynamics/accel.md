<!-- generated from accel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::accel`

The joint accelerations under given torques.

## Signature

```cpp
pluss::Series robot::accel(const robot::Robot& robot, const pluss::Series& q, const pluss::Series& qd, const pluss::Series& tau, const robot::DynamicsOptions& options = {});
```

## What It Does

Solves `M(q) qdd = tau - bias(q, qd)` for the accelerations. It is the right-hand side of the equations of motion: integrate it to simulate the robot.

## Parameters

`robot`
: The compiled robot, with mass properties.

`q`
: The joint values, `[..., links]`.

`qd`
: The joint rates, `[..., links]`.

`tau`
: The applied joint torques and forces, `[..., links]`.

`options`
: The gravity; see `DynamicsOptions`.

## Returns

A real `[..., links]` array of joint accelerations.

## Errors

Throws `pluss::Error` when the robot has no mass properties or an argument is malformed.

## Interpretation and Use

The call undoes `torques`: the accelerations that torques produce are the ones that gave those torques. Use it as the derivative function of an integrator; with `tau` zero a conservative system keeps its total energy, which is a good check on the step size.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Free fall, holding torques and a round trip.

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
  const pluss::Series q = vec({0.4, -0.8}), qd = vec({1.3, -0.7});
  // released from rest the arm falls: the accelerations are those that gravity produces
  out.writeln("free acceleration", flat(rb::accel(robot, q, vec({0, 0}), vec({0, 0}), in_plane())));
  // the torques that hold the pose give zero acceleration
  const pluss::Series hold = rb::torques(robot, q, vec({0, 0}), vec({0, 0}), in_plane());
  out.writeln("holding torques", flat(hold));
  out.writeln("acceleration under them", flat(rb::accel(robot, q, vec({0, 0}), hold, in_plane())));
  // forward and inverse dynamics undo each other
  const pluss::Series wanted = vec({0.9, -1.4});
  const pluss::Series tau = rb::torques(robot, q, qd, wanted, in_plane());
  out.writeln("recovered acceleration", flat(rb::accel(robot, q, qd, tau, in_plane())));
}
```

Output

```text
free acceleration [-16.54275, 4.64135]
holding torques [14.90875, 3.61424]
acceleration under them [0.00000, 0.00000]
recovered acceleration [0.90000, -1.40000]
```
### Example 2

A one-second simulation with energy conserved.

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
### Example 3

Against inverse dynamics.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bias.html">
    <span class="label">Previous</span>
    <span class="title">bias</span>
  </a>
  <a class="next-link" href="ke.html">
    <span class="label">Next</span>
    <span class="title">ke</span>
  </a>
</div>
<!-- apiref:nav:end -->
