<!-- generated from inverse_dynamics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Inverse dynamics

[Back to Robotics 2: Dynamics](../robot_02_dynamics.md)

**Inverse dynamics** answers: what joint torques produce this motion? `robot::torques` computes them by the recursive Newton-Euler method. This page finds the torques of the planar arm for a given acceleration and checks them against the parts of the equations of motion, computes a whole sampled motion in one call, checks a robot with a sliding joint against its closed-form equations, and shows the effect of the gravity setting.

## The mental model

The equations of motion of a robot are `M(q) qdd + b(q, qd) = tau`: the inertia matrix `M` times the accelerations, plus the **bias** `b` (Coriolis and centrifugal forces, which depend on the rates, plus gravity), equals the applied torques `tau` (forces for prismatic joints). `torques(robot, q, qd, qdd)` returns `tau`. It walks the tree from the base to the tips computing every link's velocity and acceleration, then from the tips back to the base accumulating the force and moment each joint must carry, so the cost grows linearly with the number of links.

Mass properties are required: `mass`, and `center` and `inertia` unless a point mass at the joint origin is a good model. Gravity is a vector in the base frame, default `(0, 0, -9.81)`; set it through `DynamicsOptions`.

## The torques of a motion

The planar arm lies in the x-y plane, so gravity points along `-y`. The program computes the torques for a given state and acceleration, compares them with `M qdd + bias` assembled from the parts, and then the torques that merely hold the pose.

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

The torques are `(15.243, 3.442)` N m, equal to `M qdd + bias` from `inertia` and `bias`. Holding the pose at rest takes `(14.909, 3.614)` N m, all of it gravity.

## A whole motion in one call

A leading axis is a batch, so the torques along a sampled trajectory are one call. The program samples `q(t) = (0.5 sin t, 0.3 cos t)` and its derivatives at five times.

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

The torque array has shape `[5, 2]`, time sample then joint, and the shoulder torque falls from `15.9` to `14.0` N m over the second as the arm swings.

## A sliding joint against closed forms

A cart that slides along a rail (a prismatic joint) carries a pendulum (a revolute joint). The equations of this system are classical, so the torques can be checked against closed forms: the force on the cart and the torque on the pole.

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

Both agree with the closed forms to the printed digits. For a prismatic joint the result is a force in newtons.

## The gravity setting

The default gravity points along `-z`. A robot that moves in the x-y plane needs gravity along `-y`, and a different planet or none at all is one vector away.

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
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0, 0});
  out.writeln("earth", flat(rb::bias(robot, q, still, in_plane())));
  rb::DynamicsOptions moon;
  moon.gravity = vec({0.0, -1.62, 0.0});
  out.writeln("moon ", flat(rb::bias(robot, q, still, moon)));
  rb::DynamicsOptions none;
  none.gravity = vec({0.0, 0.0, 0.0});
  out.writeln("none ", flat(rb::bias(robot, q, still, none)));
  // the default is gravity along -z: for a robot moving in the x-y plane it does nothing
  out.writeln("default gravity (0, 0, -9.81) on the planar arm", flat(rb::bias(robot, q, still)));
}
```

Output

```text
earth [14.9088, 3.6142]
moon  [2.4620, 0.5968]
none  [0.0000, 0.0000]
default gravity (0, 0, -9.81) on the planar arm [0.0000, 0.0000]
```

On the moon the same arm needs about a sixth of the torque, with zero gravity none, and with the default gravity along `-z` a planar arm in the x-y plane sees no gravity at all.

## Errors

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

Each message names the argument or the missing mass properties.

## Decisions

- **Torques for a planned trajectory:** one batched call with the trajectory's `q`, `qd` and `qdd`.
- **Torques to hold a pose:** `qd` and `qdd` zero, or `bias` at rest.
- **A robot in a plane or a different gravity:** set `DynamicsOptions::gravity`.
- **Torques of a robot with a point-mass link:** omit `center` and `inertia`.

## Pitfalls

The program makes three mistakes: leaving the default gravity on a robot that moves in the x-y plane, expecting torques to vanish with zero velocity and acceleration, and putting the centre of mass in the world frame instead of the link frame.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Links arm(std::vector<double> centers) {
  rb::Links links;
  links.parent = {-1, 0};
  links.prismatic = {0, 0};
  links.axis = mat({0, 0, 1, 0, 0, 1}, {2, 3});
  links.translation = mat({0, 0, 0, 0.5, 0, 0}, {2, 3});
  links.mass = vec({1.0, 2.0});
  links.center = mat(centers, {2, 3});
  links.inertia = mat({0, 0, 0, 0, 0, 0, 0, 0, 0.02, 0, 0, 0, 0, 0, 0, 0, 0, 0.03}, {2, 3, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series q = vec({0.4, -0.8}), still = vec({0.0, 0.0});
  rb::DynamicsOptions plane;
  plane.gravity = vec({0.0, -9.81, 0.0});

  // 1. the default gravity is along -z, which does nothing to a planar arm
  const rb::Robot good = rb::make(arm({0.25, 0, 0, 0.2, 0, 0}));
  out.writeln("holding torque, default gravity", flat(rb::torques(good, q, still, still)));
  out.writeln("holding torque, gravity along -y", flat(rb::torques(good, q, still, still, plane)));

  // 2. at rest the torques are not zero: they hold the arm up
  out.writeln("zero velocity and acceleration, torque to hold the pose", flat(rb::torques(good, q, still, still, plane)));

  // 3. the centre of mass is in the link frame: (0.25, 0, 0) is a quarter metre along the link, not a world position
  const rb::Robot wrong = rb::make(arm({0.25, 0, 0, 0.2 + 0.5, 0, 0}));
  out.writeln("second link centre entered as a world position (0.7 m)", flat(rb::torques(wrong, q, still, still, plane)));
}
```

Output

```text
holding torque, default gravity [0.0000, 0.0000]
holding torque, gravity along -y [14.9088, 3.6142]
zero velocity and acceleration, torque to hold the pose [14.9088, 3.6142]
second link centre entered as a world position (0.7 m) [23.9444, 12.6499]
```

With the default gravity the holding torque of the planar arm is `(0, 0)`, with gravity along `-y` it is `(14.909, 3.614)` N m, and the same torque is what zero velocity and acceleration need: the arm is held up, not free. Entering the second link's centre as a world position, `0.7 m`, instead of `0.2 m` in its own frame raises the holding torque to `(23.944, 12.650)` N m.

## Where next

[Mass matrix and simulation](mass_matrix_and_simulation.md) uses the parts of the equations of motion to simulate. Reference pages: [`torques`](../../../../api/eng/robot/dynamics/torques.md), [`DynamicsOptions`](../../../../api/eng/robot/dynamics/DynamicsOptions.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="mass_matrix_and_simulation.html">
    <span class="label">Next</span>
    <span class="title">Mass matrix and simulation</span>
  </a>
</div>
<!-- apiref:nav:end -->
