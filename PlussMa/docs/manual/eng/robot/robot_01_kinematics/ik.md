<!-- generated from inverse_kinematics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Inverse kinematics

[Back to Robotics 1: Kinematics](../robot_01_kinematics.md)

You know where the hand should be and you want the joint values that put it there. `robot::ik` finds them by **damped least squares**: it measures how far the link is from the target, builds the Jacobian, and moves the joints by a damped pseudo-inverse step, repeating until the error is below a tolerance. This page solves position and pose targets on a six-joint arm, solves a batch of targets in one call, shows what an unreachable target looks like, and shows the controls of the iteration.

## The mental model

Each iteration computes the task error `e` (the position error, with a weighted orientation error appended when a target rotation is given), takes the Jacobian `J` of the link, and moves the joints by `J^T (J J^T + lambda^2 I)^-1 e`. The damping `lambda` keeps the step finite near singular configurations, where a plain pseudo-inverse would demand enormous joint motions. The orientation error is `0.5 (R_target R^T - R R_target^T)` read as a vector: the sine of the turning angle along the turning axis.

Three things follow.

- **The answer depends on the start.** An arm with more joints than the task needs, or one with several arm configurations that reach the same point, has many solutions. The iteration returns the one near `q0`.
- **A result is not a promise.** `converged` says whether the error got below `tolerance`; an unreachable target returns the last iterate with `converged` false, which is not a meaningful configuration.
- **The solution carries no sensitivity.** The iteration runs off the autodiff tape. Differentiate through `frames` at the solution instead.

## A position target

A six-joint arm: a column that turns, then alternating shoulder, elbow and wrist joints. The program picks a reachable target by running forward kinematics on a known configuration, then solves for it from a different start.

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

The iteration converges in `4` iterations and the reached position matches the target to the printed six decimals.

## A position and orientation target

Adding a target rotation asks the hand to point the right way as well. The program uses the pose of the same known configuration.

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

Both the position and the orientation errors are below `1e-6` after `4` iterations.

## Several targets at once

Leading axes of the target and the start are a batch, solved together.

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

The result has shapes `[2, 6]` for the joint values and `[2]` for the error and the convergence flags, and both targets converge; the `iterations` field is the number of iterations the whole batch needed.

## A target out of reach

The arm is about `1.1 m` long; a target `5 m` away cannot be reached. The iteration runs to its limit and says so.

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

`converged` is false and `100` iterations were used. The error, `7.6 m`, is the distance left where the iteration stopped: the nearest the arm gets to a target that is `8.7 m` from the base, and since steps that do not reduce the error are rejected it never exceeds the starting error.

## The controls

`IkOptions` has `max_iterations`, `tolerance`, `damping` and `orientation_weight`. A larger damping takes smaller steps and needs more of them; a looser tolerance stops sooner; an `orientation_weight` of zero ignores the orientation.

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

At damping `1e-3` the solve takes `4` iterations, at `0.1` it takes `7` and at `1.0` it takes `14`: damping is the starting value of a Levenberg-Marquardt parameter that is halved after every accepted step and quadrupled after a rejected one, so a large damping only costs a few extra iterations. The tolerance moves the count only slightly, from `3` to `4` iterations over eight orders of magnitude, because Newton-like convergence is fast near the answer.

## What the result holds

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const rb::Robot robot = rb::make(six_links());
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const pluss::Series goal = rb::frames(robot, truth).position.select(0, 5);
  const rb::IkResult solved = rb::ik(robot, 5, goal, pluss::Series{}, vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  out.writeln("q", flat(solved.q));
  out.writeln("error", solved.error.item<double>());
  out.writeln("iterations", solved.iterations.items<std::int64_t>()[0]);
  out.writeln("converged", solved.converged.item<bool>());
  // a different joint solution can reach the same point: the result is not the configuration the target came from
  out.writeln("the generating configuration", flat(truth));
}
```

Output

```text
q [0.34067344, 0.52095237, -0.62792564, 0.22576117, 0.68882993, -0.10000000]
error 0.00000000
iterations 4
converged true
the generating configuration [0.40000000, 0.50000000, -0.60000000, 0.30000000, 0.70000000, -0.20000000]
```

The joint values found are not those the target was generated from: a six-joint arm reaching a point has more than one way to do it.

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

## Decisions

- **A position target:** leave `target_rotation` undefined; the arm may point any way.
- **A pose target:** give a rotation matrix; set `orientation_weight` to trade orientation accuracy against position accuracy.
- **A trajectory of targets:** solve target by target, starting each from the previous solution so the arm moves continuously; or batch the targets when their solutions are independent.
- **Near a singularity:** the damping adapts by itself; raise the starting `damping` only if the first steps are wild.
- **Joint limits:** set `lower` and `upper` in `IkOptions`; a target the limits exclude returns the nearest reachable configuration with `converged` false.

## Pitfalls

The program makes three mistakes: trusting a result without checking `converged`, starting from a configuration far from the wanted solution, and asking for an orientation that the arm cannot combine with the position.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace rb = pluss::subsp::eng::robot;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static rb::Links six() {
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
  const rb::Robot robot = rb::make(six());
  const pluss::Series truth = vec({0.4, 0.5, -0.6, 0.3, 0.7, -0.2});
  const rb::Pose goal = rb::frames(robot, truth);
  const pluss::Series position = goal.position.select(0, 5);

  // 1. a result that did not converge still has joint values: read the flag
  const rb::IkResult far = rb::ik(robot, 5, vec({3.0, 0.0, 0.0}), pluss::Series{}, vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0}));
  out.writeln("target out of reach: converged", far.converged.item<bool>(), " error", far.error.item<double>());

  // 2. different starts reach the same point with different joint values
  const rb::IkResult a = rb::ik(robot, 5, position, pluss::Series{}, vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  const rb::IkResult b = rb::ik(robot, 5, position, pluss::Series{}, vec({-0.4, -0.5, 0.6, -0.3, -0.7, 0.2}));
  out.writeln("start near the truth ", a.converged.item<bool>(), flat(a.q));
  out.writeln("start far from it    ", b.converged.item<bool>(), flat(b.q));

  // 3. a rotation that does not go with the position: the pose of a different configuration
  const rb::Pose other = rb::frames(robot, vec({-0.5, 0.2, 0.4, -0.8, 0.1, 0.6}));
  const rb::IkResult mixed = rb::ik(robot, 5, position, other.rotation.select(0, 5), vec({0.3, 0.4, -0.5, 0.2, 0.6, -0.1}));
  out.writeln("position of one pose, orientation of another: converged", mixed.converged.item<bool>(), " error", mixed.error.item<double>());
}
```

Output

```text
target out of reach: converged false  error 2.1150
start near the truth  true [0.3407, 0.5210, -0.6279, 0.2258, 0.6888, -0.1000]
start far from it     true [-0.2453, 0.0366, 0.5055, -0.4890, -0.6598, 0.2000]
position of one pose, orientation of another: converged true  error 0.0000
```

A target `3 m` out of reach leaves `converged` false and an error of `2.1 m`, the nearest the arm gets. Two starts reach the same position with different joints: `(0.341, 0.521, -0.628, 0.226, 0.689, -0.100)` from near the generating configuration and `(-0.245, 0.037, 0.506, -0.489, -0.660, 0.200)` from far away, both converged. A position and an orientation taken from two different poses are, for this six-joint arm, still reachable together (`converged` true, error zero): a six-joint arm can place and orient the hand independently, so check `converged` rather than assuming a target is infeasible.

## Where next

[Robotics 2: Dynamics](../robot_02_dynamics.md) relates joint motion to joint forces. Reference pages: [`ik`](../../../../api/eng/robot/kinematics/ik.md), [`IkOptions`](../../../../api/eng/robot/kinematics/IkOptions.md), [`IkResult`](../../../../api/eng/robot/kinematics/IkResult.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="forward_kinematics_and_jacobians.html">
    <span class="label">Previous</span>
    <span class="title">Forward kinematics and Jacobians</span>
  </a>
</div>
<!-- apiref:nav:end -->
