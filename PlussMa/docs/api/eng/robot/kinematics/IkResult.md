<!-- generated from IkResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `robot::IkResult`

The result of inverse kinematics.

## Signature

```cpp
robot::IkResult
```

## What It Does

Holds the joint values found, how far the link still is from the target, how many iterations were used and whether every member of the batch converged.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `q` | The joint values, `[..., links]`. |  |
| `error` | The norm of the remaining task error of each target, `[...]`; the position error in metres, with the weighted orientation error appended when an orientation was requested. |  |
| `iterations` | The number of iterations used, an `Int64` vector of one element. The batch iterates together until every member has converged. |  |
| `converged` | Whether the error of each target is within the tolerance, a Bool array `[...]`. |  |

## Returns

Returned by `ik`.

## Errors

Reading a field does not iterate again.

## Interpretation and Use

Check `converged` before using `q`: an unreachable target (or one excluded by joint limits) returns the nearest configuration the iteration reached, with `converged` false: the error never grows from the start. The solution is one of possibly many; it is generally not the configuration the target was generated from.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Every field of a solution.

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
### Example 2

A batch of targets.

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
### Example 3

An unreachable target.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IkOptions.html">
    <span class="label">Previous</span>
    <span class="title">IkOptions</span>
  </a>
  <a class="next-link" href="ik.html">
    <span class="label">Next</span>
    <span class="title">ik</span>
  </a>
</div>
<!-- apiref:nav:end -->
