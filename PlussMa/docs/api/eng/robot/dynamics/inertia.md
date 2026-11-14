<!-- generated from inertia.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::inertia`

The joint-space inertia matrix.

## Signature

```cpp
pluss::Series robot::inertia(const robot::Robot& robot, const pluss::Series& q);
```

## What It Does

Returns the matrix `M(q)` for which the kinetic energy is `0.5 qd^T M qd` and the torques of a motion are `M qdd` plus the bias. Column `j` is computed as the torque that gives a unit acceleration at joint `j` with no velocity and no gravity.

## Parameters

`robot`
: The compiled robot, with mass properties.

`q`
: The joint values, a real floating `[..., links]` array.

## Returns

A real `[..., links, links]` array, symmetric and positive definite for a robot with positive masses and inertias.

## Errors

Throws `pluss::Error` when the robot has no mass properties or `q` is not a real floating `[..., links]` array on the robot's platform.

## Interpretation and Use

The matrix does not depend on joint rates or gravity. In a serial chain the inertia seen at a joint depends on the joints farther out, not on those nearer the base: the planar arm's matrix does not depend on the shoulder angle. It is what to invert for forward dynamics and the inertia a controller sees.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The planar-arm inertia matrix against its closed form.

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
### Example 2

The inertia as the elbow bends.

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
  // the inertia as the elbow bends from straight to folded
  const pluss::Series q = mat({0.0, 0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 3.0}, {4, 2});
  const pluss::Series m = rb::inertia(robot, q);
  out.writeln("mass matrix shape", m.sizes(), " (configuration, joint, joint)");
  out.writeln("shoulder inertia M11 as the elbow bends", flat(m.select(1, 0).select(1, 0)));
  out.writeln("coupling M12 as the elbow bends        ", flat(m.select(1, 0).select(1, 1)));
}
```

Output

```text
mass matrix shape [4, 2, 2]  (configuration, joint, joint)
shoulder inertia M11 as the elbow bends [1.0925, 0.9086, 0.5260, 0.2965]
coupling M12 as the elbow bends         [0.3100, 0.2181, 0.0268, -0.0880]
```
### Example 3

Kinetic energy from the matrix.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="torques.html">
    <span class="label">Previous</span>
    <span class="title">torques</span>
  </a>
  <a class="next-link" href="bias.html">
    <span class="label">Next</span>
    <span class="title">bias</span>
  </a>
</div>
<!-- apiref:nav:end -->
