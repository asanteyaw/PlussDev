<!-- generated from torques.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::torques`

The joint forces and torques of a motion.

## Signature

```cpp
pluss::Series robot::torques(const robot::Robot& robot, const pluss::Series& q, const pluss::Series& qd, const pluss::Series& qdd, const robot::DynamicsOptions& options = {});
```

## What It Does

Computes the joint torques (forces for prismatic joints) that produce the accelerations `qdd` at the state `(q, qd)` against gravity, by the recursive Newton-Euler method: velocities and accelerations are propagated from the base outwards, the force and moment on each link are computed from its mass properties, and they are accumulated from the tips back to the base.

## Parameters

`robot`
: The compiled robot, with mass properties.

`q`
: The joint values, a real floating `[..., links]` array.

`qd`
: The joint rates, `[..., links]`.

`qdd`
: The joint accelerations, `[..., links]`.

`options`
: The gravity; see `DynamicsOptions`.

## Returns

A real `[..., links]` array of joint torques, in newton-metres for revolute joints and newtons for prismatic ones.

## Errors

Throws `pluss::Error` when the robot has no mass properties, when `q`, `qd` or `qdd` is not a real floating `[..., links]` array on the robot's platform, or when the gravity is malformed.

## Interpretation and Use

The result equals `M(q) qdd + bias(q, qd)`. With `qd` and `qdd` zero it is the torque that holds the pose against gravity. The cost is linear in the number of links and the call is vectorized over a batch, so a whole trajectory of torques is one call. The result carries reverse sensitivity to the configuration and the mass properties.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Torques of a planar arm against M qdd plus bias.

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

A sampled motion in one call.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(arm_links());
  // a sampled motion q(t) = (0.5 sin t, 0.3 cos t): the torque history in one call
  std::vector<double> q, qd, qdd;
  for (int k = 0; k < 5; ++k) {
    const double t = 0.25 * k;
    q.insert(q.end(), {0.5 * std::sin(t), 0.3 * std::cos(t)});
    qd.insert(qd.end(), {0.5 * std::cos(t), -0.3 * std::sin(t)});
    qdd.insert(qdd.end(), {-0.5 * std::sin(t), -0.3 * std::cos(t)});
  }
  const pluss::Series tau = rb::torques(robot, mat(q, {5, 2}), mat(qd, {5, 2}), mat(qdd, {5, 2}), in_plane());
  out.writeln("torque shape", tau.sizes(), " (time sample, joint)");
  out.writeln("shoulder torque", flat(tau.select(1, 0)));
  out.writeln("elbow torque   ", flat(tau.select(1, 1)));
}
```

Output

```text
torque shape [5, 2]  (time sample, joint)
shoulder torque [15.9209, 15.5438, 15.0170, 14.4498, 13.9643]
elbow torque    [3.7305, 3.5361, 3.3464, 3.2016, 3.1314]
```
### Example 3

A cart with a pendulum against closed forms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a cart on a rail carrying a pendulum: the cart joint slides along x, the pendulum turns about z
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {1, 0};
  links.axis = mat({1, 0, 0, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0, 0, 0}, {2, 3});
  links.mass = vec({1.5, 0.7});
  links.center = mat({0, 0, 0, 0.6, 0, 0}, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.01}, {2, 3, 3});
  const rb::Robot robot = rb::make(links);
  rb::DynamicsOptions options;
  options.gravity = vec({0.0, -9.81, 0.0});
  const double th = 0.9, dth = 1.7, ax = 0.4, ath = -2.2;
  const pluss::Series tau = rb::torques(robot, vec({0.3, th}), vec({-0.8, dth}), vec({ax, ath}), options);
  // closed forms of the cart-pole equations
  const double m = 0.7, M = 1.5, l = 0.6, spin = 0.01;
  out.writeln("force on the cart   ", flat(tau)[0], " closed form", (M + m) * ax - m * l * std::sin(th) * ath - m * l * std::cos(th) * dth * dth);
  out.writeln("torque on the pole  ", flat(tau)[1], " closed form", -m * l * std::sin(th) * ax + (m * l * l + spin) * ath + m * 9.81 * l * std::cos(th));
}
```

Output

```text
force on the cart    0.84928  closed form 0.84928
torque on the pole   1.85316  closed form 1.85316
```
### Example 4

Rejected arguments.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  const rb::Robot robot = rb::make(arm_links());
  report([&] { (void)rb::torques(robot, vec({0.1, 0.2}), vec({0.1}), vec({0.1, 0.2}), in_plane()); });
  report([&] { (void)rb::torques(robot, vec({0.1, 0.2}), vec({0.1, 0.2}), vec({0.1, 0.2, 0.3}), in_plane()); });
  rb::Links links = arm_links();
  links.mass = pluss::Series{};
  links.center = pluss::Series{};
  links.inertia = pluss::Series{};
  report([&] { (void)rb::torques(rb::make(links), vec({0.1, 0.2}), vec({0.1, 0.2}), vec({0.1, 0.2})); });
  rb::DynamicsOptions bad;
  bad.gravity = vec({0.0, -9.81});
  report([&] { (void)rb::torques(robot, vec({0.1, 0.2}), vec({0.1, 0.2}), vec({0.1, 0.2}), bad); });
}
```

Output

```text
eng::robot::torques: qd must be a real floating [..., 2] array
eng::robot::torques: qdd must be a real floating [..., 2] array
eng::robot::torques: the robot has no mass properties; set mass (and optionally center and inertia) in Links
eng::robot: gravity must be a real floating vector of three components
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DynamicsOptions.html">
    <span class="label">Previous</span>
    <span class="title">DynamicsOptions</span>
  </a>
  <a class="next-link" href="inertia.html">
    <span class="label">Next</span>
    <span class="title">inertia</span>
  </a>
</div>
<!-- apiref:nav:end -->
