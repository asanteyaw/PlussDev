<!-- generated from condensed_mpc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The finite-horizon problem

[Back to Control 4: Model-Predictive Control](../control_04_model_predictive_control.md)

LQR gives the optimal feedback when nothing is limited. Real actuators saturate, and real systems have states that must stay inside a range. **Model-predictive control** (MPC) handles both: at each step it solves an optimisation over the next `N` steps with the limits as constraints, applies the first move, and repeats. This page covers the optimisation itself: `mpc::solve` computes the best sequence of inputs for a given state, and `mpc::build` prepares the problem once so it can be solved quickly for many states. The next page closes the loop.

## The mental model

For the discrete plant `x[k+1] = A x[k] + B u[k]` and the current state `x0`, MPC chooses the inputs `u[0], ..., u[N-1]` to minimise

```{math}
\sum_{k=1}^{N-1} (x_k - r_k)^T Q\,(x_k - r_k) + (x_N - r_N)^T Q_f\,(x_N - r_N) + \sum_{k=0}^{N-1} u_k^T R\,u_k
```

subject to `u_min <= u_k <= u_max` and, optionally, `x_min <= x_k <= x_max`. Here `x_1, ..., x_N` are the *predicted* states (the current state `x0` is given, not penalised), `r_k` is a reference, `Q` weights the states, `R` the inputs, and `Q_f` the last state. Substituting the model `x_k = A^k x0 + sum_j A^(k-1-j) B u_j` eliminates the states and leaves a **quadratic program** in the `N m` inputs alone, with the state limits becoming linear inequalities on those inputs. That is the **condensed** form the solver works on.

The inputs are:

| Argument | Meaning |
|---|---|
| `system` | a discrete `StateSpace` (a continuous plant must be discretised first) |
| `Weights{state, input, terminal}` | the matrices `Q` (positive semidefinite), `R` (positive definite) and `Q_f` (optional; defaults to `Q`) |
| `Bounds{input_lower, input_upper, state_lower, state_upper}` | the limits. The input bounds are required; the state bounds are optional but come as a pair |
| `Options{horizon, qp}` | the horizon `N` (default 10) and the settings of the quadratic-program solver |
| `initial_state` | `x0`, shape `[states]`, or `[..., states]` for a batch of start states |
| `reference` | either one vector `[states]`, held for the whole horizon, or a trajectory `[horizon, states]` |

Bounds, too, may be a single vector (the same at every step) or a `[horizon, ...]` trajectory (different limits at different steps).

The result is a `Result` with four members:

| Member | Shape | Meaning |
|---|---|---|
| `control` | `[inputs]` | the first move `u[0]`, the one that is applied |
| `controls` | `[horizon, inputs]` | the whole optimal input sequence |
| `states` | `[horizon, states]` | the predicted states `x_1, ..., x_N` |
| `optimization` | | the quadratic-program result: `converged`, `iterations`, `primal_residual`, `dual_residual`, `objective` |

The program below uses the sampled double integrator throughout this chapter: `x = [position, velocity]`, a force input, sample time `0.1`, `A = [[1, 0.1], [0, 1]]`, `B = [0.005, 0.1]^T`.

## MPC contains LQR

Without active limits MPC is LQR: if the terminal weight `Q_f` is the Riccati solution `P` of the infinite-horizon problem, the finite-horizon optimum is the infinite-horizon one for any `N`, and the first move is `-K x0`. The program computes `P` and `K` with `dare`, solves a 20-step problem with very loose bounds, and prints the first controls beside the LQR sequence. The two sequences are equal to the printed digits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);

  mpc::Options options;
  options.horizon = 20;
  const mpc::Weights weights{.state = q, .input = r, .terminal = lqr.value};
  const mpc::Bounds loose{.input_lower = vec({-100.0}), .input_upper = vec({100.0})};
  const pluss::Series start = vec({1.0, 0.0});
  const auto result = mpc::solve(plant, start, vec({0.0, 0.0}), weights, loose, options);

  std::vector<double> lqr_controls;
  pluss::Series x = start.unsqueeze(-1);
  for (int k = 0; k < 5; ++k) {
    const pluss::Series u = -lqr.gain.matmul(x);
    lqr_controls.push_back(u.squeeze().item<double>());
    x = plant.a().matmul(x) + plant.b().matmul(u);
  }
  out.writeln("shapes: control", result.control.sizes(), " controls", result.controls.sizes(), " states", result.states.sizes());
  out.writeln("MPC controls, first five", result.controls.slice(0, 0, 5).reshape({5}).items<double>());
  out.writeln("LQR controls, first five", lqr_controls);
  out.writeln("converged", result.optimization.converged, " iterations", result.optimization.iterations, " primal residual", result.optimization.primal_residual);
}
```

Output

```text
shapes: control [1]  controls [20, 1]  states [20, 2]
MPC controls, first five [-2.58570, -1.66190, -1.00129, -0.53373, -0.20732]
LQR controls, first five [-2.58570, -1.66190, -1.00129, -0.53373, -0.20732]
converged true  iterations 20  primal residual 0.00001
```

## The terminal weight matters

If no terminal weight is given, the last state is weighted by `Q` like the others, and a short horizon then undervalues where the state ends up: the controller is too timid. The program solves the same problem with horizons 2, 5 and 20 and `Q_f = Q`. The first move starts at `-0.164` with `N = 2` and approaches the LQR value `-2.586` only as the horizon grows (`-2.430` at `N = 20`). With `Q_f = P` all three horizons give `-2.586` (as in the previous program). For any problem that is meant to approximate the infinite-horizon optimum, take the terminal weight from `dare`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);
  const mpc::Bounds loose{.input_lower = vec({-100.0}), .input_upper = vec({100.0})};

  out.writeln("LQR first move", (-lqr.gain.matmul(vec({1.0, 0.0}).unsqueeze(-1))).squeeze().item<double>());
  for (const long horizon : {2L, 5L, 20L}) {
    mpc::Options options;
    options.horizon = horizon;
    const auto plain = mpc::solve(plant, vec({1.0, 0.0}), vec({0.0, 0.0}), mpc::Weights{.state = q, .input = r}, loose, options);
    const auto tuned = mpc::solve(plant, vec({1.0, 0.0}), vec({0.0, 0.0}), mpc::Weights{.state = q, .input = r, .terminal = lqr.value}, loose, options);
    out.writeln("horizon", horizon, " terminal = Q:", plain.control.reshape({1}).items<double>(), " terminal = P:", tuned.control.reshape({1}).items<double>());
  }
}
```

Output

```text
LQR first move -2.58570
horizon 2  terminal = Q: [-0.16377]  terminal = P: [-2.58570]
horizon 5  terminal = Q: [-0.70400]  terminal = P: [-2.58570]
horizon 20  terminal = Q: [-2.43015]  terminal = P: [-2.58570]
```

## Input bounds

With `|u| <= 0.5` the same problem has a different optimum. The unconstrained first move is `-2.586`, far beyond the limit, so the optimiser saturates the inputs for as long as it pays to. The program solves the 20-step problem with the bounds and prints the first eight controls, the largest control magnitude anywhere in the sequence, and the predicted final state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);

  mpc::Options options;
  options.horizon = 20;
  const mpc::Weights weights{.state = q, .input = r, .terminal = lqr.value};
  const mpc::Bounds bounds{.input_lower = vec({-0.5}), .input_upper = vec({0.5})};
  const auto result = mpc::solve(plant, vec({1.0, 0.0}), vec({0.0, 0.0}), weights, bounds, options);

  out.writeln("first eight controls", result.controls.slice(0, 0, 8).reshape({8}).items<double>());
  out.writeln("largest |u| in the sequence", result.controls.abs().max().item<double>());
  out.writeln("predicted final state", result.states.select(0, 19).reshape({2}).items<double>());
  out.writeln("converged", result.optimization.converged, " iterations", result.optimization.iterations);
}
```

Output

```text
first eight controls [-0.50000, -0.50000, -0.50000, -0.50000, -0.50000, -0.50000, -0.50000, -0.50000]
largest |u| in the sequence 0.50000
predicted final state [0.30543, -0.30901]
converged true  iterations 40
```

The first eight controls are all `-0.50000`: the input is pinned at its lower bound while the mass is still far from the origin, and the sequence never exceeds the bound. Twenty steps (2 seconds) are not enough to reach the origin at this effort, so the predicted final state is still at position `0.305`, moving at `-0.309`.

## Different limits at different steps

A bound given as `[horizon, inputs]` varies with the step. The program tightens the limit from `0.5` to `0.1` for the last five of ten steps, as when an actuator is going to be taken out of service. The optimal sequence follows the bound: five moves at `-0.5`, then five at `-0.1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);

  std::vector<double> lower(10, -0.5), upper(10, 0.5);
  for (int k = 5; k < 10; ++k) {
    lower[k] = -0.1;
    upper[k] = 0.1;
  }
  mpc::Options options;
  options.horizon = 10;
  const mpc::Bounds bounds{.input_lower = pluss::make_series(lower, {10, 1}, f64), .input_upper = pluss::make_series(upper, {10, 1}, f64)};
  const auto result = mpc::solve(plant, vec({1.0, 0.0}), vec({0.0, 0.0}), mpc::Weights{.state = q, .input = r, .terminal = lqr.value}, bounds, options);
  out.writeln("controls", result.controls.reshape({10}).items<double>());
}
```

Output

```text
controls [-0.50000, -0.50000, -0.50000, -0.50000, -0.50000, -0.10000, -0.10000, -0.10000, -0.10000, -0.10000]
```

## Tracking a reference

The cost penalises the distance of the predicted states from `reference`. A vector reference is held constant; a `[horizon, states]` reference is a trajectory. The first program moves the mass from rest at the origin to position 1 (reference `[1, 0]`); the second asks it to follow a ramp, position `0.05 k` at velocity `0.5`. The predicted position follows the reference: at the end of the horizon it has reached `0.815` of the 1 asked for in the first case (the horizon is too short to arrive), and `0.969` against `1.0` in the second.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);
  mpc::Options options;
  options.horizon = 20;
  const mpc::Weights weights{.state = q, .input = r, .terminal = lqr.value};
  const mpc::Bounds loose{.input_lower = vec({-100.0}), .input_upper = vec({100.0})};
  const pluss::Series rest = vec({0.0, 0.0});

  const auto constant = mpc::solve(plant, rest, vec({1.0, 0.0}), weights, loose, options);
  out.writeln("constant reference [1, 0]: first move", constant.control.reshape({1}).items<double>(), " final predicted state", constant.states.select(0, 19).reshape({2}).items<double>());

  std::vector<double> ramp;
  for (int k = 0; k < 20; ++k) {
    ramp.push_back(0.05 * (k + 1));
    ramp.push_back(0.5);
  }
  const auto trajectory = mpc::solve(plant, rest, pluss::make_series(ramp, {20, 2}, f64), weights, loose, options);
  out.writeln("ramp reference: predicted position at steps 5, 10, 20:", trajectory.states.select(0, 4).select(0, 0).item<double>(),
              trajectory.states.select(0, 9).select(0, 0).item<double>(), trajectory.states.select(0, 19).select(0, 0).item<double>());
  out.writeln("                reference position at steps 5, 10, 20:", 0.25, 0.5, 1.0);
}
```

Output

```text
constant reference [1, 0]: first move [2.58570]  final predicted state [0.81473, 0.19389]
ramp reference: predicted position at steps 5, 10, 20: 0.15411 0.42185 0.96896
                reference position at steps 5, 10, 20: 0.25000 0.50000 1.00000
```

## Errors

The inputs are validated before the problem is assembled. The program shows a continuous system, a non-positive horizon, a singular `R`, a non-symmetric and an indefinite `Q`, state bounds given singly, crossed bounds, an initial state of the wrong width, a reference of the wrong width and a bound of the wrong width.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series a = matrix({1, 0.1, 0, 1}, 2, 2), b = matrix({0.005, 0.1}, 2, 1), c = matrix({1, 0}, 1, 2), d = matrix({0}, 1, 1);
  const auto plant = lti::state(a, b, c, d, 0.1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({0.1}, 1, 1);
  const mpc::Weights weights{.state = q, .input = r};
  const mpc::Bounds loose{.input_lower = vec({-1.0}), .input_upper = vec({1.0})};
  mpc::Options options;
  options.horizon = 10;
  mpc::Options none;
  none.horizon = 0;

  report([&] { (void)mpc::build(lti::state(a, b, c, d), weights, loose, options); });
  report([&] { (void)mpc::build(plant, weights, loose, none); });
  report([&] { (void)mpc::build(plant, mpc::Weights{.state = q, .input = matrix({0.0}, 1, 1)}, loose, options); });
  report([&] { (void)mpc::build(plant, mpc::Weights{.state = matrix({1, 2, 0, 1}, 2, 2), .input = r}, loose, options); });
  report([&] { (void)mpc::build(plant, mpc::Weights{.state = matrix({-1, 0, 0, 1}, 2, 2), .input = r}, loose, options); });
  report([&] { (void)mpc::build(plant, weights, mpc::Bounds{.input_lower = vec({-1.0}), .input_upper = vec({1.0}), .state_lower = vec({-1.0, -1.0})}, options); });
  report([&] { (void)mpc::build(plant, weights, mpc::Bounds{.input_lower = vec({1.0}), .input_upper = vec({-1.0})}, options); });
  report([&] { (void)mpc::build(plant, weights, mpc::Bounds{.input_lower = vec({-1.0, -1.0}), .input_upper = vec({1.0})}, options); });
  const auto plan = mpc::build(plant, weights, loose, options);
  report([&] { (void)mpc::solve(plan, vec({1.0}), vec({0.0, 0.0})); });
  report([&] { (void)mpc::solve(plan, vec({1.0, 0.0}), vec({0.0})); });
}
```

Output

```text
eng::control::mpc::solve: system must be discrete
eng::control::mpc::solve: horizon must be positive
eng::control::mpc::solve: weights.input must be positive definite
eng::control::mpc::solve: weights.state must be symmetric
eng::control::mpc::solve: weights.state must be positive semidefinite
eng::control::mpc::solve: state bounds must be supplied together
eng::control::mpc::solve: bounds.input_lower must not exceed bounds.input_upper
eng::control::mpc::solve: bounds.input_lower has the wrong trailing width
eng::control::mpc::solve: initial_state must end in [states]
eng::control::mpc::solve: reference has the wrong trailing width
```

## Decisions

- **Horizon:** long enough to see the consequences of a move: a few times the slowest closed-loop time constant, in steps. A longer horizon costs more per solve; check `optimization.iterations`.
- **Terminal weight:** `dare`'s `P` when the plant is stabilisable, so that the finite horizon matches the infinite one beyond it.
- **Bounds:** put every physical limit on inputs and states in `Bounds`. A limit that you enforce afterwards by clipping is not seen by the optimiser.
- **A discretised plant:** MPC needs a discrete model; choose the sample time from the dynamics (several samples per rise time).

## Pitfalls

The program makes four mistakes with a condensed MPC problem on the sampled double integrator: leaving the terminal weight at its default, setting the input weight to zero, reading `states[0]` as the current state, and applying a result that did not converge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace mpc = pluss::subsp::eng::control::mpc;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto report = [&](const char *label, const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(std::string(label) + ": " + text);
    }
  };
  const auto plant = lti::state(matrix({1, 0.1, 0, 1}, 2, 2), matrix({0.005, 0.1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1), 0.1);
  const pluss::Series q = pluss::series::eye(2, f64), r = matrix({0.1}, 1, 1);
  const auto lqr = ctl::dare(plant.a(), plant.b(), q, r);
  const mpc::Bounds loose{.input_lower = vec({-100.0}), .input_upper = vec({100.0})};
  const pluss::Series start = vec({1.0, 0.0}), zero = vec({0.0, 0.0});
  const double lqr_move = flat(-lqr.gain.matmul(start.unsqueeze(-1)))[0];

  // 1. the default terminal weight is Q: a short horizon is timid
  mpc::Options shorter;
  shorter.horizon = 5;
  const auto timid = mpc::solve(plant, start, zero, mpc::Weights{.state = q, .input = r}, loose, shorter);
  const auto set = mpc::solve(plant, start, zero, mpc::Weights{.state = q, .input = r, .terminal = lqr.value}, loose, shorter);
  out.writeln("first move, horizon 5: default terminal weight ", flat(timid.control)[0], "   terminal weight P ", flat(set.control)[0], "   the LQR move -K x0 ", lqr_move);

  // 2. R must be positive definite
  report("R = 0", [&] { (void)mpc::solve(plant, start, zero, mpc::Weights{.state = q, .input = matrix({0.0}, 1, 1)}, loose, shorter); });

  // 3. states[0] is x_1, the state after the first move
  const auto a = flat(plant.a()), b = flat(plant.b());
  const double u0 = flat(set.control)[0];
  out.writeln("x0 = ", flat(start), "   states[0] = ", flat(set.states.select(0, 0)), "   A x0 + B u0 = ", std::vector<double>{a[0] * 1.0 + a[1] * 0.0 + b[0] * u0, a[2] * 1.0 + a[3] * 0.0 + b[1] * u0});

  // 4. a result that did not converge is not to be applied
  mpc::Options starved = shorter;
  starved.qp.max_iters = 10;
  const mpc::Bounds tight{.input_lower = vec({-0.5}), .input_upper = vec({0.5})};
  const auto cut = mpc::solve(plant, start, zero, mpc::Weights{.state = q, .input = r, .terminal = lqr.value}, tight, starved);
  const auto done = mpc::solve(plant, start, zero, mpc::Weights{.state = q, .input = r, .terminal = lqr.value}, tight, shorter);
  out.writeln("10 iterations: converged ", cut.optimization.converged, ", primal residual ", cut.optimization.primal_residual, ", first move ", flat(cut.control)[0]);
  out.writeln("default budget: converged ", done.optimization.converged, ", primal residual ", done.optimization.primal_residual, ", first move ", flat(done.control)[0]);
}
```

Output

```text
first move, horizon 5: default terminal weight -0.70400   terminal weight P -2.58570   the LQR move -K x0 -2.58570
R = 0: eng::control::mpc::solve: weights.input must be positive definite
x0 = [1.00000, 0.00000]   states[0] = [0.98707, -0.25857]   A x0 + B u0 = [0.98707, -0.25857]
10 iterations: converged false, primal residual 0.06137, first move -0.56137
default budget: converged true, primal residual 0.00000, first move -0.50000
```

With a horizon of `5` and the default terminal weight, `Q`, the first move is `-0.70400`; with the terminal weight set to the Riccati solution `P` it is `-2.58570`, the LQR move `-K x0`, so the default horizon is timid and the weight is worth setting. A zero input weight is refused, since free control makes the problem unbounded. `states[0]` is `x_1`, the state after the first move: it is `[0.98707, -0.25857]`, which is `A x0 + B u0`, not the `x0` that was passed in. And a result that did not converge is a partial answer: after `10` iterations the problem with a force limit of `0.5` has `converged` false, a primal residual of `0.06137` and a first move of `-0.56137`, outside the limit; with the default budget it converges, with a residual of `0.00000` and a move of exactly `-0.50000`. Check `converged` before applying the move.

Reference: [`state`](../../../../api/eng/lti/state/state.md), [`dare`](../../../../api/eng/control/optimal/dare.md).

Where next: [Receding-horizon control](receding_horizon_control.md) applies the first move at each step and solves again, with the problem prepared once.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="receding_horizon_control.html">
    <span class="label">Next</span>
    <span class="title">Receding-horizon control</span>
  </a>
</div>
<!-- apiref:nav:end -->
