<!-- generated from riccati_equations_and_lqr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Riccati equations and LQR

[Back to Control 1: Optimal Feedback](../control_01_optimal_feedback.md)

You have a plant and you want a feedback law `u = -K x` that keeps its states small without spending too much effort on the input. The linear-quadratic regulator (LQR) gives the gain that minimises a quadratic cost, and it is found by solving an algebraic **Riccati equation**. `control::lqr` does it from a state-space model, and `care` and `dare` solve the continuous and discrete equations from the raw matrices. This page shows what the weights mean, how to read the result, and the checks that confirm the gain is optimal.

## The mental model

For a continuous plant `x' = A x + B u` the LQR minimises

```{math}
J = \int_0^\infty \left( x^T Q x + 2\,x^T N u + u^T R u \right) dt,
```

where `Q` (positive semidefinite) says how much you mind each state being away from zero, `R` (positive definite) says how much you mind each unit of input, and the optional **cross term** `N` couples the two. The optimal law is `u = -K x` with

```{math}
K = R^{-1}\left(B^T P + N^T\right),
```

where `P` is the positive semidefinite solution of the continuous algebraic Riccati equation

```{math}
A^T P + P A - (P B + N) R^{-1} (B^T P + N^T) + Q = 0.
```

For a discrete plant `x[k+1] = A x[k] + B u[k]` the cost is the sum of the same terms over `k`, the equation is

```{math}
P = A^T P A - (A^T P B + N)(R + B^T P B)^{-1}(B^T P A + N^T) + Q,
```

and `K = (R + B^T P B)^{-1}(B^T P A + N^T)`. In both cases `P` is more than a by-product: **the minimum cost from the state `x0` is `x0^T P x0`**. The closed-loop matrix is `A - B K`; the solution chosen is the one that makes it stable.

The functions return a `RiccatiResult` with three members: `value`, the matrix `P`; `gain`, the matrix `K`; and `closed_loop`, `A - B K`.

| Call | Solves |
|---|---|
| `care(A, B, Q, R)` | the continuous equation |
| `dare(A, B, Q, R)` | the discrete equation |
| `lqr(system, Q, R)` | `care` or `dare`, chosen by the model's sample time |
| each of the three with an extra `cross` argument | the same with the cross term `N` |

## One state, solved by hand

The plant `x' = x + u` is unstable: left alone, `x` grows as `e^t`. With `Q = R = 1` the Riccati equation is `2P - P^2 + 1 = 0`, whose positive root is `P = 1 + sqrt(2) = 2.414214`. Then `K = P`, and the closed loop is `x' = (1 - K) x = -sqrt(2) x`, stable with time constant `0.707`. The program solves it with `care` and compares with the hand calculation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto result = ctl::care(scalar(1.0), scalar(1.0), scalar(1.0), scalar(1.0));
  out.writeln("P (value)       ", result.value.squeeze().item<double>(), " hand: 1 + sqrt(2) =", 1.0 + std::sqrt(2.0));
  out.writeln("K (gain)        ", result.gain.squeeze().item<double>());
  out.writeln("A - B K         ", result.closed_loop.squeeze().item<double>(), " hand: -sqrt(2) =", -std::sqrt(2.0));
}
```

Output

```text
P (value)        2.414214  hand: 1 + sqrt(2) = 2.414214
K (gain)         2.414214
A - B K          -1.414214  hand: -sqrt(2) = -1.414214
```

The three numbers match the hand calculation. The gain is above the minimum needed to stabilise the plant (`K > 1`), which is the point of the cost: it buys a closed-loop pole at `-1.41` instead of leaving the plant marginally stable at `K = 1`.

## Choosing the weights

`R` sets the price of control. Cheap control (small `R`) gives a large gain and a fast closed loop; expensive control gives a gain barely above the stabilising minimum. The program repeats the scalar problem with `R = 0.01, 1, 100`: as `R` grows, `K` falls towards `1` and the closed-loop pole towards `0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln("   R       gain K       closed-loop pole");
  for (const double r : {0.01, 1.0, 100.0}) {
    const auto result = ctl::care(scalar(1.0), scalar(1.0), scalar(1.0), scalar(r));
    out.writeln(r, result.gain.squeeze().item<double>(), result.closed_loop.squeeze().item<double>());
  }
}
```

Output

```text
   R       gain K       closed-loop pole
0.010000 11.049876 -10.049876
1.000000 2.414214 -1.414214
100.000000 2.004988 -1.004988
```

With `R = 0.01` the gain is `11.05` and the pole `-10.05`: the controller is willing to spend a lot of input to make `x` die quickly. With `R = 100` the gain is `2.005` and the pole `-1.005`, just inside the stable side. The pole never crosses `-1`, because the plant's own unstable pole at `+1` must at least be mirrored.

## Two states: the double integrator

A frictionless mass driven by a force has `x1' = x2`, `x2' = u`: `A = [[0, 1], [0, 0]]`, `B = [[0], [1]]`. It cannot be stabilised by doing nothing, and it is the standard test for a regulator. With `Q = I` and `R = 1` the Riccati equation has the exact solution `P = [[sqrt(3), 1], [1, sqrt(3)]]`, so `K = [1, sqrt(3)]` and the closed loop `s^2 + sqrt(3) s + 1` has the damping ratio `sqrt(3)/2 = 0.866`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series a = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series b = matrix({0, 1}, 2, 1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({1}, 1, 1);

  const auto result = ctl::care(a, b, q, r);
  out.writeln("P                ", result.value.reshape({4}).items<double>());
  out.writeln("K                ", result.gain.reshape({2}).items<double>(), " hand: [1, sqrt(3)] =", std::sqrt(3.0));
  out.writeln("closed loop A-BK ", result.closed_loop.reshape({4}).items<double>());

  const pluss::Series p = result.value;
  const pluss::Series residual = a.transpose(0, 1).matmul(p) + p.matmul(a) - p.matmul(b).matmul(b.transpose(0, 1)).matmul(p) + q;
  out.writeln("residual of the Riccati equation, largest entry:", residual.abs().max().item<double>());
}
```

Output

```text
P                 [1.732051, 1.000000, 1.000000, 1.732051]
K                 [1.000000, 1.732051]  hand: [1, sqrt(3)] = 1.732051
closed loop A-BK  [0.000000, 1.000000, -1.000000, -1.732051]
residual of the Riccati equation, largest entry: 0.000000
```

The residual is the check that matters: it evaluates the left-hand side of the Riccati equation with the returned `P`, and it is zero to the printed digits. The closed-loop matrix `[[0, 1], [-1, -sqrt(3)]]` has the characteristic polynomial `s^2 + sqrt(3) s + 1`.

## LQR from a model

`lqr(system, Q, R)` takes the plant as a `StateSpace` and calls `care` for a continuous model and `dare` for a discrete one, so the same line of code serves both. The program builds the double integrator as a model and confirms that it agrees with `care`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series a = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series b = matrix({0, 1}, 2, 1);
  const auto plant = lti::state(a, b, matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({1}, 1, 1);

  const auto from_model = ctl::lqr(plant, q, r);
  const auto from_matrices = ctl::care(a, b, q, r);
  out.writeln("K from lqr ", from_model.gain.reshape({2}).items<double>());
  out.writeln("largest difference from care:", (from_model.gain - from_matrices.gain).abs().max().item<double>());
}
```

Output

```text
K from lqr  [1.000000, 1.732051]
largest difference from care: 0.000000
```

Only `A` and `B` enter the design; `C` and `D` are about what is measured, not what is controlled.

## A discrete plant

For `x[k+1] = 1.2 x[k] + u[k]`, an unstable discrete plant, with `Q = R = 1`, `dare` solves the discrete equation. The program prints `P`, `K` and the closed-loop pole, checks the discrete Riccati equation by hand, and shows that `lqr` on a model with a sample time reaches the same answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto result = ctl::dare(scalar(1.2), scalar(1.0), scalar(1.0), scalar(1.0));
  const double p = result.value.squeeze().item<double>();
  out.writeln("P", p, " K", result.gain.squeeze().item<double>(), " closed-loop pole", result.closed_loop.squeeze().item<double>());
  out.writeln("residual 1.44 P - 1.44 P^2 / (1 + P) + 1 - P =", 1.44 * p - 1.44 * p * p / (1.0 + p) + 1.0 - p);

  const auto plant = lti::state(scalar(1.2), scalar(1.0), scalar(1.0), scalar(0.0), 0.1);
  out.writeln("lqr on a model with sample time 0.1: K", ctl::lqr(plant, scalar(1.0), scalar(1.0)).gain.squeeze().item<double>());
}
```

Output

```text
P 1.952234  K 0.793528  closed-loop pole 0.406472
residual 1.44 P - 1.44 P^2 / (1 + P) + 1 - P = 0.000000
lqr on a model with sample time 0.1: K 0.793528
```

The closed-loop pole `0.406` is inside the unit circle: the unstable plant (`1.2`) has been stabilised. The discrete cost does not depend on the sample time `0.1`; the sample time only chooses `dare` over `care`.

## The cross term

A cross term `N` penalises products of states and inputs, which arises when the cost is written in other coordinates or includes the effect of a measured disturbance. The call takes it as a fifth argument, and the optimum is `K = R^{-1}(B^T P + N^T)`. The program adds `N = [0.2, 0]^T` to the double integrator and checks the generalised Riccati equation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series a = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series b = matrix({0, 1}, 2, 1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({1}, 1, 1);
  const pluss::Series n = matrix({0.2, 0.0}, 2, 1);

  const auto plain = ctl::care(a, b, q, r);
  const auto coupled = ctl::care(a, b, q, r, n);
  out.writeln("K without the cross term", plain.gain.reshape({2}).items<double>());
  out.writeln("K with the cross term   ", coupled.gain.reshape({2}).items<double>());

  const pluss::Series p = coupled.value;
  const pluss::Series pb_n = p.matmul(b) + n;
  const pluss::Series residual = a.transpose(0, 1).matmul(p) + p.matmul(a) - pb_n.matmul(pb_n.transpose(0, 1)) + q;
  out.writeln("residual of the generalised equation:", residual.abs().max().item<double>());
}
```

Output

```text
K without the cross term [1.000000, 1.732051]
K with the cross term    [1.000000, 1.612452]
residual of the generalised equation: 0.000000
```

The cross term changes the gain from `[1, 1.732]` to `[1, 1.612]`. With `R = 1`, the generalised Riccati equation's left-hand side is `A^T P + P A - (P B + N)(P B + N)^T + Q`, and the returned `P` makes it zero.

## Batches of plants

`A` may carry leading batch axes, and the solution has the same leading axes, so a family of plants is designed in one call. The program takes the double integrator and a variant with a restoring spring (`A[1][0] = -1`) as a batch of two.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series free_mass = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series sprung = matrix({0, 1, -1, 0}, 2, 2);
  const pluss::Series both = free_mass.unsqueeze(0).unite({sprung.unsqueeze(0)}, 0);

  const auto result = ctl::care(both, matrix({0, 1}, 2, 1), pluss::series::eye(2, f64), matrix({1}, 1, 1));
  out.writeln("value shape", result.value.sizes(), " gain shape", result.gain.sizes());
  out.writeln("gain, free mass", result.gain.select(0, 0).reshape({2}).items<double>());
  out.writeln("gain, sprung   ", result.gain.select(0, 1).reshape({2}).items<double>());
}
```

Output

```text
value shape [2, 2, 2]  gain shape [2, 1, 2]
gain, free mass [1.000000, 1.732051]
gain, sprung    [0.414214, 1.352193]
```

The spring already pulls the state back, so the second plant needs less position feedback: its gain `[0.4142, 1.3522]` is smaller than `[1, 1.7321]`.

## Errors

The weights are validated before anything is solved: `Q` must be symmetric (Hermitian) and positive semidefinite, `R` positive definite, and the joint cost with a cross term positive semidefinite. A plant with an unstable mode the input cannot reach has no stabilising solution, and `care` says so. The program shows a non-square `A`, a `B` with the wrong number of rows, a `Q` of the wrong shape, a non-symmetric `Q`, a negative `Q`, a zero `R`, a negative `R`, a cross term of the wrong shape, an indefinite joint cost, and an unstabilisable plant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

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
  const pluss::Series a = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series b = matrix({0, 1}, 2, 1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series r = matrix({1}, 1, 1);

  report([&] { (void)ctl::care(matrix({1, 2, 3, 4, 5, 6}, 2, 3), b, q, r); });
  report([&] { (void)ctl::care(a, matrix({1}, 1, 1), q, r); });
  report([&] { (void)ctl::care(a, b, matrix({1, 0, 0, 1, 0, 0}, 3, 2), r); });
  report([&] { (void)ctl::care(a, b, matrix({1, 2, 0, 1}, 2, 2), r); });
  report([&] { (void)ctl::care(a, b, matrix({-1, 0, 0, 1}, 2, 2), r); });
  report([&] { (void)ctl::care(a, b, q, matrix({0}, 1, 1)); });
  report([&] { (void)ctl::care(a, b, q, matrix({-1}, 1, 1)); });
  report([&] { (void)ctl::care(a, b, q, r, matrix({1, 1, 1}, 3, 1)); });
  report([&] { (void)ctl::care(a, b, q, r, matrix({5, 0}, 2, 1)); });
  report([&] { (void)ctl::care(matrix({1, 0, 0, -1}, 2, 2), b, q, r); });
}
```

Output

```text
eng::control::care: A must be square
eng::control::care: B rows must equal the state count
eng::control::care: Q must have shape [..., states, states]
eng::control::care: Q must be Hermitian
eng::control::care: Q must be positive semidefinite
eng::control::care: R must be positive definite
eng::control::care: R must be positive definite
eng::control::care: cross must have shape [..., states, inputs]
eng::control::care: the joint [Q, cross; cross^H, R] cost must be positive semidefinite
eng::control::care: no stabilising solution; (A, B) must be stabilisable (every unstable mode reachable from the input) and the states weighted by Q must be detectable. Solver message: subsp::math::linalg::lu_solve: singular matrix
```

The last call has an unstable mode (`+1`) that the input does not reach, so no feedback can stabilise it; the message names the two conditions that a design needs: the unstable modes must be reachable from the input (stabilisable), and the states weighted by `Q` must be detectable.

## Decisions

- **Weights:** start with `Q` and `R` diagonal, set from the largest acceptable value of each variable (`Q_ii = 1 / x_max_i^2`, `R_jj = 1 / u_max_j^2`), then scale `R` up or down to trade state error against effort.
- **Continuous or discrete:** use the model you have; `lqr` chooses. If a continuous design will be implemented digitally, simulate the digital loop before trusting it.
- **A cross term:** only if your cost really has one. For an ordinary regulator leave it out.
- **Which function:** `lqr` for a model, `care`/`dare` when you have matrices and no model (for example, a Riccati equation that arises inside another algorithm).

## Pitfalls

The program makes four mistakes with the weights and the structure of an LQR problem: scaling both weights and expecting a change, setting `R` to zero, weighting an output as if it were a state, and applying the gain to the measured output.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series scalar(double value) { return matrix({value}, 1, 1); }
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
  const auto a = matrix({0, 1, 0, 0}, 2, 2), b = matrix({0, 1}, 2, 1);
  const auto q = pluss::series::eye(2, f64);

  // 1. Q and R are weights: only their ratio matters
  out.writeln("Q = I,    R = 1:     K = ", flat(ctl::care(a, b, q, scalar(1.0)).gain));
  out.writeln("Q = 10 I, R = 10:    K = ", flat(ctl::care(a, b, q * 10.0, scalar(10.0)).gain), "   (the same weights, ten times larger)");
  out.writeln("Q = I,    R = 0.1:   K = ", flat(ctl::care(a, b, q, scalar(0.1)).gain), "   (a different ratio)");

  // 2. R must be positive definite
  report("R = 0", [&] { (void)ctl::care(a, b, q, scalar(0.0)); });

  // 3. the cost is about states: an output weight must be carried through C
  const auto c = matrix({1, 0}, 1, 2);
  report("an output weight of 1 x 1 given as Q", [&] { (void)ctl::care(a, b, scalar(1.0), scalar(1.0)); });
  const auto weighted = c.transpose(0, 1).matmul(scalar(4.0)).matmul(c);
  out.writeln("Q = C' Q_y C with Q_y = 4:  K = ", flat(ctl::care(a, b, weighted, scalar(1.0)).gain), "   Q = ", flat(weighted));

  // 4. the gain multiplies the whole state; the output has one entry
  const auto gain = ctl::care(a, b, q, scalar(1.0)).gain;
  report("K applied to a measured output of one entry", [&] { (void)gain.matmul(scalar(0.5)); });
  out.writeln("K is ", gain.sizes(), ": it needs both states, position and velocity");
}
```

Output

```text
Q = I,    R = 1:     K = [1.00000, 1.73205]
Q = 10 I, R = 10:    K = [1.00000, 1.73205]   (the same weights, ten times larger)
Q = I,    R = 0.1:   K = [3.16228, 4.04037]   (a different ratio)
R = 0: eng::control::care: R must be positive definite
an output weight of 1 x 1 given as Q: eng::control::care: Q must have shape [..., states, states]
Q = C' Q_y C with Q_y = 4:  K = [2.00000, 2.00000]   Q = [4.00000, 0.00000, 0.00000, 0.00000]
K applied to a measured output of one entry: matmul: shape mismatch
K is [1, 2]: it needs both states, position and velocity
```

Only the ratio of `Q` to `R` matters: ten times both weights give exactly the same gain `[1, 1.7321]`, and a smaller `R = 0.1` is a different design, `[3.1623, 4.0404]`. A free input, `R = 0`, would give an infinite gain and is refused. The cost is about the states, so an output weight must be carried through `C`: a `1 x 1` weight is refused as `Q`, and `Q = C^T Q_y C` with `Q_y = 4` is the matrix with `4` on the position and `0` elsewhere, which gives `[2, 2]`. And the gain is `1 x 2`, for the two states: applied to a single measured output it fails with a shape error, so when you measure less than the whole state, put an estimator between the plant and the gain.

Reference: [`care`](../../../../api/eng/control/optimal/care.md), [`state`](../../../../api/eng/lti/state/state.md), [`lqr`](../../../../api/eng/control/optimal/lqr.md), [`dare`](../../../../api/eng/control/optimal/dare.md).

Where next: [Closed-loop behaviour](closed_loop_behaviour.md) simulates the controlled system and verifies that the cost is `x0^T P x0`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="closed_loop_behaviour.html">
    <span class="label">Next</span>
    <span class="title">Closed-loop behaviour</span>
  </a>
</div>
<!-- apiref:nav:end -->
