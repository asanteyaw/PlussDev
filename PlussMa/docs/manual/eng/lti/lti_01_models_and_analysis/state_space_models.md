<!-- generated from state_space_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# State-space models

[Back to Systems 1: Models and Analysis](../lti_01_models_and_analysis.md)

You have a dynamic system, a mass on a spring, a circuit, a plant to be controlled, and you want to describe it in a form that every other tool in this library accepts. `lti::state` builds a **state-space model** from four matrices. This page explains what the matrices mean, how continuous and discrete models differ, how models of the same size are batched, and what is checked when you build one.

## The mental model

A linear time-invariant system keeps a vector of **states** `x` that summarises everything the past has done to it. An **input** vector `u` acts on the states, and an **output** vector `y` is read from them. In continuous time

```{math}
\dot x = A x + B u, \qquad y = C x + D u,
```

and in discrete time, with a fixed sampling interval `T`,

```{math}
x_{k+1} = A x_k + B u_k, \qquad y_k = C x_k + D u_k.
```

The four matrices fix the shapes of everything: with `n` states, `m` inputs and `p` outputs, `A` is `n x n`, `B` is `n x m`, `C` is `p x n` and `D` is `p x m`. `StateSpace` stores them and reports `states()`, `inputs()` and `outputs()`. Whether the model is continuous or discrete is carried by its **sample time**: `0` (the default) is continuous, any positive number is a discrete model sampled at that interval.

The matrices are leading-batch aware. Every matrix may carry extra leading axes, so one object can hold a whole family of systems, for example the same plant with several parameter values, and the other tools process them in one call.

## A mass on a spring

A unit mass on a spring of stiffness `k = 2` with a damper of coefficient `c = 3`, pushed by a force `u`, obeys `x'' + 3 x' + 2 x = u`. Taking the position and velocity as states, `x1 = x` and `x2 = x'`, gives `x1' = x2` and `x2' = -2 x1 - 3 x2 + u`. The output is the position. Read off the matrices: `A = [[0, 1], [-2, -3]]`, `B = [[0], [1]]`, `C = [[1, 0]]`, `D = [[0]]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto spring = lti::state(matrix({0, 1, -2, -3}, 2, 2), matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));

  out.writeln("states", spring.states(), " inputs", spring.inputs(), " outputs", spring.outputs());
  out.writeln("continuous", spring.is_cont(), " discrete", spring.is_disc(), " sample time", spring.sample_time());
  out.writeln("A", spring.a().reshape({4}).items<double>());
  out.writeln("B", spring.b().reshape({2}).items<double>(), " C", spring.c().reshape({2}).items<double>(), " D", spring.d().reshape({1}).items<double>());
}
```

Output

```text
states 2  inputs 1  outputs 1
continuous true  discrete false  sample time 0.000
A [0.000, 1.000, -2.000, -3.000]
B [0.000, 1.000]  C [1.000, 0.000]  D [0.000]
```

The model has two states and one input and one output. It is continuous because no sample time was given. The accessors `a()`, `b()`, `c()` and `d()` return the stored matrices, in the common element type of the four inputs.

## A discrete model

A first-order discrete system `x[k+1] = 0.5 x[k] + u[k]`, observed through `y = x`, sampled once a second. Giving a sample time marks the model as discrete, which changes how the other tools treat it: the frequency response is evaluated on the unit circle, and a simulation steps rather than integrates.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto discrete = lti::state(scalar(0.5), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  out.writeln("sample time", discrete.sample_time(), " discrete", discrete.is_disc(), " continuous", discrete.is_cont());

  const auto fast = lti::state(scalar(0.5), scalar(1.0), scalar(1.0), scalar(0.0), 0.01);
  out.writeln("same matrices at a 10 ms interval: sample time", fast.sample_time());
}
```

Output

```text
sample time 1.000  discrete true  continuous false
same matrices at a 10 ms interval: sample time 0.010
```

The matrices alone do not say what the interval is, so give it when you build the model. The same `A` with `T = 1` and `T = 0.01` is two different physical systems.

## Several inputs and outputs

The shapes follow the rule above for any `n`, `m` and `p`. The program builds a system with two states, three inputs and three outputs; `D` is the zero matrix `3 x 3`. The controllability and observability matrices that [the next page](controllability_and_gramians.md) introduces have shapes `n x (n m)` and `(n p) x n`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto wide = lti::state(matrix({-1, 0, 0, -2}, 2, 2), matrix({1, 0, 0, 0, 1, 1}, 2, 3), matrix({1, 0, 0, 1, 1, 1}, 3, 2),
                               pluss::series::zeros({3, 3}, f64));
  out.writeln("states", wide.states(), " inputs", wide.inputs(), " outputs", wide.outputs());
  out.writeln("B shape", wide.b().sizes(), " C shape", wide.c().sizes(), " D shape", wide.d().sizes());
  out.writeln("controllability matrix", lti::ctrb(wide).sizes(), " observability matrix", lti::obsv(wide).sizes());
}
```

Output

```text
states 2  inputs 3  outputs 3
B shape [2, 3]  C shape [3, 2]  D shape [3, 3]
controllability matrix [2, 6]  observability matrix [6, 2]
```

## Batches of systems

A matrix with extra leading axes is a batch. Matrices of different batch shapes are broadcast against each other, so a single `B`, `C` and `D` can be shared by a batch of `A` matrices. The program holds two spring models that differ only in stiffness (`k = 2` and `k = 8`) as one object.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  const pluss::Series soft = matrix({0, 1, -2, -3}, 2, 2);
  const pluss::Series stiff = matrix({0, 1, -8, -3}, 2, 2);
  const pluss::Series both = soft.unsqueeze(0).unite({stiff.unsqueeze(0)}, 0);

  const auto family = lti::state(both, matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  out.writeln("A", family.a().sizes(), " B", family.b().sizes(), " C", family.c().sizes(), " D", family.d().sizes());
  out.writeln("states", family.states(), " inputs", family.inputs(), " outputs", family.outputs());
}
```

Output

```text
A [2, 2, 2]  B [2, 2, 1]  C [2, 1, 2]  D [2, 1, 1]
states 2  inputs 1  outputs 1
```

The shared matrices are expanded to the batch, so every accessor has the same leading axes. The counts `states()`, `inputs()` and `outputs()` describe one system of the family.

## Element types

The matrices are promoted to a common element type, so single-precision and double-precision matrices may be mixed (the result is double), and complex matrices make a complex model. The element type must be floating point or complex.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  const auto single = pluss::ScalarType::Float32;
  const auto mixed = lti::state(scalar(-1.0).to(single), scalar(1.0).to(single), scalar(1.0), scalar(0.0).to(single));
  out.writeln("float32, float32, float64, float32 ->", mixed.a().dtype().name);

  const auto all_single = lti::state(scalar(-1.0).to(single), scalar(1.0).to(single), scalar(1.0).to(single), scalar(0.0).to(single));
  out.writeln("all float32 ->", all_single.a().dtype().name);

  const pluss::Series rotating = pluss::make_series(std::vector<std::complex<double>>{{-1.0, 2.0}}, {1, 1});
  const auto complex_model = lti::state(rotating, scalar(1.0), scalar(1.0), scalar(0.0));
  out.writeln("a complex A ->", complex_model.a().dtype().name, " B becomes", complex_model.b().dtype().name);
}
```

Output

```text
float32, float32, float64, float32 -> float64
all float32 -> float32
a complex A -> complex128  B becomes complex128
```

## Errors

Building a model checks every shape before anything else, and names the offending matrix. The program shows a matrix that is not square, a `B` with the wrong number of rows, a `C` with the wrong number of columns, a `D` of the wrong shape, a negative sample time, and a vector where a matrix is needed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

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
  const pluss::Series one = matrix({1}, 1, 1);
  report([&] { (void)lti::state(matrix({1, 2, 3, 4, 5, 6}, 2, 3), matrix({1, 0}, 2, 1), matrix({1, 0}, 1, 2), one); });
  report([&] { (void)lti::state(one, matrix({1, 0}, 2, 1), one, one); });
  report([&] { (void)lti::state(one, one, matrix({1, 0}, 1, 2), one); });
  report([&] { (void)lti::state(one, one, one, matrix({0, 0}, 1, 2)); });
  report([&] { (void)lti::state(one, one, one, one, -1.0); });
  report([&] { (void)lti::state(pluss::series::make(std::vector<double>{1.0}), one, one, one); });
}
```

Output

```text
eng::lti::state: A must be square
eng::lti::state: B rows must equal the state count
eng::lti::state: C columns must equal the state count
eng::lti::state: D must have shape [outputs, inputs]
eng::lti::state: sample_time must be finite and non-negative
eng::lti::state: A must have at least two dimensions
```

## Decisions

- **Choosing the states:** any set of variables that determines the future given the input. Physical variables (position, velocity, charge, flux) are the easiest to check by hand; the model is not unique, since any invertible change of variables describes the same system.
- **Continuous or discrete:** continuous when you have differential equations, discrete when you have a sampled controller or a difference equation. Pass the sample time when the model is discrete, and leave it at `0` otherwise.
- **One system or a batch:** use a batch when you compare parameter values or run a Monte Carlo over plants; the analysis and simulation tools accept the batch as it is.

## Pitfalls

Each mistake below is one a first model tends to contain. The program makes three: giving a continuous model's matrices a sample time and expecting the same system, passing `B` as a plain vector, and reading a state as if it were the output.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  OutputEngine out;
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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));

  // 1. a sample time is not a conversion: the same matrices mean a different system
  const auto a = matrix({-1.0}, 1, 1), b = matrix({0.0}, 1, 1), c = matrix({1.0}, 1, 1), d = matrix({0.0}, 1, 1);
  const auto continuous = lti::state(a, b, c, d);
  const auto discrete = lti::state(a, b, c, d, 1.0);
  const auto time = pluss::series::make(std::vector<double>{0.0, 1.0, 2.0, 3.0});
  const auto input = pluss::series::zeros({4, 1}, f64);
  const auto start = pluss::series::make(std::vector<double>{1.0});
  out.writeln("x' = -x from x = 1:       ", flat(lti::sim(continuous, input, time, start).values), "   (decays, e^-t)");
  out.writeln("x[k+1] = -x[k] from x = 1:", flat(lti::sim(discrete, input, time, start).values), "   (the same matrices, a different system)");

  // 2. B, C and D are matrices
  report("B given as a vector", [&] { (void)lti::state(a, pluss::series::make(std::vector<double>{0.0}), c, d); });

  // 3. the output is not the state
  const auto spring = lti::state(matrix({0, 1, -2, -3}, 2, 2), matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  const auto run = lti::sim(spring, pluss::series::zeros({3, 1}, f64), pluss::series::make(std::vector<double>{0.0, 0.5, 1.0}), pluss::series::make(std::vector<double>{1.0, 0.0}));
  out.writeln("output (position only):", flat(run.values));
  out.writeln("states (position, velocity) per time:", flat(run.states));
}
```

Output

```text
x' = -x from x = 1:       [1.0000, 0.3679, 0.1353, 0.0498]   (decays, e^-t)
x[k+1] = -x[k] from x = 1:[1.0000, -1.0000, 1.0000, -1.0000]   (the same matrices, a different system)
B given as a vector: eng::lti::state: B must have at least two dimensions
output (position only):[1.0000, 0.8452, 0.6004]
states (position, velocity) per time:[1.0000, 0.0000, 0.8452, -0.4773, 0.6004, -0.4651]
```

A sample time is part of what the matrices mean. `x' = -x` decays as `e^-t`, but the same matrices with a sample time of `1` are `x[k+1] = -x[k]`, which flips sign forever. The sample time cannot be changed afterwards either, so to discretise a continuous model, compute the discrete matrices and build a new model from them. `B`, `C` and `D` must be matrices: a vector `B` is refused with a message that names the matrix. And `y = C x + D u` is only what `C` selects: the output of the spring is the position alone, `1.0, 0.8452, 0.6004`, while the velocity, `0.0, -0.4773, -0.4651`, is in the states and not in the output; to read a state directly, make `C` select it.

Reference: [`state`](../../../../api/eng/lti/state/state.md), [`ctrb`](../../../../api/eng/lti/state/ctrb.md), [`obsv`](../../../../api/eng/lti/state/obsv.md).

Where next: [Controllability and gramians](controllability_and_gramians.md) asks whether the inputs can move the states and whether the outputs reveal them.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="controllability_and_gramians.html">
    <span class="label">Next</span>
    <span class="title">Controllability and gramians</span>
  </a>
</div>
<!-- apiref:nav:end -->
