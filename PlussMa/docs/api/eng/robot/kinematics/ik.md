<!-- generated from ik.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::ik`

Joint values that bring a link to a target.

## Signature

```cpp
robot::IkResult robot::ik(const robot::Robot& robot, std::int64_t link, const pluss::Series& target_position, const pluss::Series& target_rotation, const pluss::Series& q0, const robot::IkOptions& options = {});
```

## What It Does

Finds joint values for which the frame of `link` reaches `target_position` and, when `target_rotation` is defined, `target_rotation`, by Levenberg-Marquardt damped least squares: each iteration measures the error, builds the Jacobian and moves the joints by the damped pseudo-inverse step, accepting it only if the error falls (and otherwise raising the damping), within the joint limits of `options` if any. The orientation error is `0.5 (R_target R^T - R R_target^T)` read as a vector, which is the sine of the turning angle along the turning axis.

## Parameters

`robot`
: The compiled robot.

`link`
: The index of the link whose frame is to reach the target.

`target_position`
: The wanted position of the link's origin in the world, a real floating `[..., 3]` array.

`target_rotation`
: The wanted orientation, `[..., 3, 3]`, or an undefined `Series` for a position-only target.

`q0`
: The starting joint values, `[..., links]`. The solution found depends on the start.

`options`
: The iteration controls; see `IkOptions`.

## Returns

An `IkResult` with the joint values, the remaining error, the iteration count and a convergence flag per target.

## Errors

Throws `pluss::Error` for a `link` outside the robot, a `target_position`, `target_rotation` or `q0` of the wrong shape, dtype or platform, or invalid `options`. A target that cannot be reached is not an error: the result reports `converged` false.

## Interpretation and Use

The iteration runs without recording the autodiff tape and the solution carries no sensitivity. Choose `q0` near the configuration you expect; a redundant robot (more joints than the task needs) has a family of solutions and the iteration finds the nearest. A batch of targets and starts is solved together.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A position target for a six-joint arm.

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
  // choose a reachable target by running the forward kinematics on a known configuration
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const pluss::Series goal = rb::frames(robot, truth).position.select(0, 5);
  const rb::IkResult solved = rb::ik(robot, 5, goal, pluss::Series{}, vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  out.writeln("converged", solved.converged.item<bool>(), " iterations", solved.iterations.items<std::int64_t>()[0]);
  out.writeln("target       ", flat(goal));
  out.writeln("reached      ", flat(rb::frames(robot, solved.q).position.select(0, 5)));
  out.writeln("remaining error below 1e-8", solved.error.item<double>() < 1e-8);
}
```

Output

```text
converged true  iterations 4
target        [0.212893, 0.033385, 1.123725]
reached       [0.212893, 0.033385, 1.123725]
remaining error below 1e-8 true
```
### Example 2

A position-and-orientation target.

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
  const rb::IkResult solved = rb::ik(robot, 5, goal.position.select(0, 5), goal.rotation.select(0, 5),
                                                      vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  const rb::Pose reached = rb::frames(robot, solved.q);
  out.writeln("converged", solved.converged.item<bool>(), " iterations", solved.iterations.items<std::int64_t>()[0]);
  out.writeln("position error below 1e-6", reached.position.select(0, 5).sub(goal.position.select(0, 5)).abs().amax().item<double>() < 1e-6);
  out.writeln("orientation error below 1e-6", reached.rotation.select(0, 5).sub(goal.rotation.select(0, 5)).abs().amax().item<double>() < 1e-6);
}
```

Output

```text
converged true  iterations 4
position error below 1e-6 true
orientation error below 1e-6 true
```
### Example 3

Two targets in one call.

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
  // two targets and two starting configurations solved together
  const pluss::Series truths = mat({0.4, 0.5, -0.6, 0.3, 0.7, -0.2, -0.3, 0.6, -0.4, 0.2, 0.5, 0.1}, {2, 6});
  const pluss::Series goals = rb::frames(robot, truths).position.select(1, 5);
  const pluss::Series starts = mat({0.3, 0.4, -0.5, 0.2, 0.6, -0.1, -0.2, 0.5, -0.3, 0.1, 0.4, 0.0}, {2, 6});
  const rb::IkResult solved = rb::ik(robot, 5, goals, pluss::Series{}, starts);
  out.writeln("q shape", solved.q.sizes(), " error shape", solved.error.sizes(), " converged shape", solved.converged.sizes());
  out.writeln("converged", flat(solved.converged.to(pluss::ScalarType::Float64)));
  out.writeln("iterations used by the batch", solved.iterations.items<std::int64_t>()[0]);
}
```

Output

```text
q shape [2, 6]  error shape [2]  converged shape [2]
converged [1.000000, 1.000000]
iterations used by the batch 4
```
### Example 4

A target that is out of reach.

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
### Example 5

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
### Example 6

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
  <a class="prev-link" href="IkResult.html">
    <span class="label">Previous</span>
    <span class="title">IkResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
