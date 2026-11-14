<!-- generated from IkOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::IkOptions`

Controls of the inverse-kinematics iteration.

## Signature

```cpp
robot::IkOptions
```

## What It Does

Sets when the damped least-squares iteration of `ik` stops and how it takes its steps.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `max_iterations` | The largest number of iterations, at least `1`. | `100` |
| `tolerance` | The iteration stops when the largest error norm over the batch is below `tolerance` (metres, with the orientation error in the same units). | `1e-8` |
| `damping` | The starting Levenberg damping `lambda`, positive. A step solves `(J J^T + lambda^2 I) y = e` and moves the joints by `J^T y`; the damping is then halved after an accepted step and quadrupled after a rejected one, per target, so a larger value only starts the iteration more cautiously. | `1e-3` |
| `orientation_weight` | The weight of the orientation error against the position error when a target rotation is given; `0` ignores the orientation. | `1` |
| `lower` | The lower joint limit of each link, a real floating vector with one value per link, in the units of the joints (radians or metres). The iteration never goes below it; undefined means no lower limit. | undefined |
| `upper` | The upper joint limit of each link, in the same form. The iteration never goes above it; undefined means no upper limit. | undefined |

## Returns

A value; read by `ik`.

## Errors

`ik` throws `pluss::Error` for `max_iterations` below `1`, a non-positive or non-finite `tolerance` or `damping`, a negative or non-finite `orientation_weight`, joint limits that are not real floating vectors with one value per link, or a lower limit above its upper limit.

## Interpretation and Use

Convergence is quadratic near a solution with the default damping, so tightening the tolerance adds an iteration or two. Steps that do not reduce the error are rejected, so the error never grows. Joint limits are applied to every step: a target that the limits exclude is not reached, and `converged` is false.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Damping, tolerance and orientation weight against the iterations used.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a six-joint arm: a column, then alternating shoulder, elbow and wrist joints
static rb::Links six_links() {
  rb::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = mat({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const rb::Pose goal = rb::frames(robot, truth);
  const pluss::Series start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  // a larger damping takes shorter, safer steps; a looser tolerance stops sooner
  for (double damping : {1e-3, 1e-1, 1.0}) {
    rb::IkOptions options;
    options.damping = damping;
    const rb::IkResult solved = rb::ik(robot, 5, goal.position.select(0, 5), pluss::Series{}, start, options);
    out.writeln("damping", damping, " iterations", solved.iterations.items<std::int64_t>()[0], " converged", solved.converged.item<bool>());
  }
  for (double tolerance : {1e-4, 1e-8, 1e-12}) {
    rb::IkOptions options;
    options.tolerance = tolerance;
    const rb::IkResult solved = rb::ik(robot, 5, goal.position.select(0, 5), pluss::Series{}, start, options);
    out.writeln("tolerance 1e", static_cast<int>(std::lround(std::log10(tolerance))), " iterations", solved.iterations.items<std::int64_t>()[0]);
  }
  // orientation_weight scales the orientation error against the position error; zero ignores the orientation
  rb::IkOptions ignore;
  ignore.orientation_weight = 0.0;
  const rb::IkResult loose = rb::ik(robot, 5, goal.position.select(0, 5), goal.rotation.select(0, 5), start, ignore);
  out.writeln("orientation_weight 0: converged", loose.converged.item<bool>(), " iterations", loose.iterations.items<std::int64_t>()[0]);
}
```

Output

```text
damping 0.001000  iterations 4  converged true
damping 0.100000  iterations 7  converged true
damping 1.000000  iterations 14  converged true
tolerance 1e -4  iterations 3
tolerance 1e -8  iterations 4
tolerance 1e -12  iterations 4
orientation_weight 0: converged true  iterations 4
```
### Example 2

A target that cannot be reached.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a six-joint arm: a column, then alternating shoulder, elbow and wrist joints
static rb::Links six_links() {
  rb::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = mat({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  return links;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rb::Robot robot = rb::make(six_links());
  // the arm is about 1.1 m long: a target 5 m away cannot be reached, and the result says so
  const rb::IkResult solved = rb::ik(robot, 5, vec({5.0, 5.0, 5.0}), pluss::Series{}, vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  out.writeln("converged", solved.converged.item<bool>());
  out.writeln("distance left to the target", solved.error.item<double>());
  out.writeln("iterations", solved.iterations.items<std::int64_t>()[0]);
}
```

Output

```text
converged false
distance left to the target 7.59058
iterations 100
```
### Example 3

Joint limits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a six-joint arm: a column, then alternating shoulder, elbow and wrist joints
static rb::Links six_links() {
  rb::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = mat({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  return links;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series goal = rb::frames(robot, vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2})).position.select(0, 5);
  const pluss::Series start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  // joint limits in radians, one pair per joint: the iteration never leaves them
  rb::IkOptions wide;
  wide.lower = vec({-1, -1, -1, -1, -1, -1});
  wide.upper = vec({1, 1, 1, 1, 1, 1});
  const rb::IkResult free = rb::ik(robot, 5, goal, pluss::Series{}, start, wide);
  out.writeln("wide limits: converged", free.converged.item<bool>(), " joint values", flat(free.q));
  rb::IkOptions tight;
  tight.lower = vec({-0.45, -0.45, -0.65, -0.35, -0.75, -0.25});
  tight.upper = vec({0.45, 0.55, 0.65, 0.35, 0.75, 0.25});
  const rb::IkResult boxed = rb::ik(robot, 5, goal, pluss::Series{}, start, tight);
  out.writeln("tight limits: converged", boxed.converged.item<bool>(), " joint values", flat(boxed.q));
  // limits that exclude the target: the nearest reachable configuration, flagged as not converged
  rb::IkOptions blocked;
  blocked.lower = vec({-0.1, -0.1, -0.1, -0.1, -0.1, -0.1});
  blocked.upper = vec({0.1, 0.1, 0.1, 0.1, 0.1, 0.1});
  const rb::IkResult stuck = rb::ik(robot, 5, goal, pluss::Series{}, vec({0, 0, 0, 0, 0, 0}), blocked);
  out.writeln("blocked: converged", stuck.converged.item<bool>(), " distance left", stuck.error.item<double>(), " joint values", flat(stuck.q));
}
```

Output

```text
wide limits: converged true  joint values [0.3407, 0.5210, -0.6279, 0.2258, 0.6888, -0.1000]
tight limits: converged true  joint values [0.3416, 0.5200, -0.6268, 0.2271, 0.6913, -0.1000]
blocked: converged false  distance left 0.0893  joint values [0.0225, 0.1000, 0.1000, -0.1000, 0.1000, 0.0000]
```
### Example 4

Rejected controls.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a six-joint arm: a column, then alternating shoulder, elbow and wrist joints
static rb::Links six_links() {
  rb::Links links;
  links.parent = {-1, 0, 1, 2, 3, 4};
  links.prismatic = {0, 0, 0, 0, 0, 0};
  links.axis = mat({0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0}, {6, 3});
  links.translation = mat({0, 0, 0.2, 0, 0, 0.1, 0, 0, 0.4, 0, 0, 0.3, 0, 0, 0.1, 0, 0, 0.1}, {6, 3});
  return links;
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
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series start = vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1});
  report([&] { (void)rb::ik(robot, 6, vec({0.5, 0.0, 0.5}), pluss::Series{}, start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0}), pluss::Series{}, start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), vec({1.0, 0.0, 0.0}), start); });
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), pluss::Series{}, vec({0.1, 0.2})); });
  rb::IkOptions options;
  options.damping = 0.0;
  report([&] { (void)rb::ik(robot, 5, vec({0.5, 0.0, 0.5}), pluss::Series{}, start, options); });
}
```

Output

```text
eng::robot::ik: link is outside the robot
eng::robot::ik: target_position must be a real floating [..., 3] array
eng::robot::ik: target_rotation must be a real floating [..., 3, 3] array
eng::robot::ik: q0 must be a real floating [..., 6] array
eng::robot::ik: max_iterations must be at least 1, tolerance and damping positive, orientation_weight non-negative
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="jac.html">
    <span class="label">Previous</span>
    <span class="title">jac</span>
  </a>
  <a class="next-link" href="IkResult.html">
    <span class="label">Next</span>
    <span class="title">IkResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
