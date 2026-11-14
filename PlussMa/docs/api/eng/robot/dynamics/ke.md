<!-- generated from ke.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::ke`

The kinetic energy of a motion.

## Signature

```cpp
pluss::Series robot::ke(const robot::Robot& robot, const pluss::Series& q, const pluss::Series& qd);
```

## What It Does

Returns `0.5 qd^T M(q) qd` for every configuration and rate of the batch.

## Parameters

`robot`
: The compiled robot, with mass properties.

`q`
: The joint values, `[..., links]`.

`qd`
: The joint rates, `[..., links]`.

## Returns

A real array of shape `[...]`, non-negative.

## Errors

Throws `pluss::Error` when the robot has no mass properties or an argument is malformed.

## Interpretation and Use

Together with `pe` it gives the total energy, which forward dynamics without applied torques must conserve.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Kinetic and potential energy of a planar arm.

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
  // 0.5 qd' M qd, from the mass matrix
  const pluss::Series m = rb::inertia(robot, q);
  out.writeln("kinetic energy  ", rb::ke(robot, q, qd).item<double>());
  out.writeln("0.5 qd' M qd    ", 0.5 * qd.unsqueeze(0).matmul(m).matmul(qd.unsqueeze(-1)).select(0, 0).select(0, 0).item<double>());
  // potential energy m g h summed over the link centres: with the arm in a vertical plane it falls as the arm lowers
  out.writeln("potential energy, arm raised", rb::pe(robot, vec({1.2, 0.3}), in_plane()).item<double>());
  out.writeln("potential energy, arm lowered", rb::pe(robot, vec({-1.2, -0.3}), in_plane()).item<double>());
  const pluss::Series batch = mat({1.2, 0.3, -1.2, -0.3}, {2, 2});
  out.writeln("batched potential energy shape", rb::pe(robot, batch, in_plane()).sizes());
}
```

Output

```text
kinetic energy   0.62070
0.5 qd' M qd     0.62070
potential energy, arm raised 15.34330
potential energy, arm lowered -15.34330
batched potential energy shape [2]
```
### Example 2

Total energy through a simulation.

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

The inertia matrix behind it.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="accel.html">
    <span class="label">Previous</span>
    <span class="title">accel</span>
  </a>
  <a class="next-link" href="pe.html">
    <span class="label">Next</span>
    <span class="title">pe</span>
  </a>
</div>
<!-- apiref:nav:end -->
