<!-- generated from state.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::state`

Builds a state-space model from its four matrices.

## Signature

```cpp
lti::StateSpace lti::state(
    const pluss::Series& a,
    const pluss::Series& b,
    const pluss::Series& c,
    const pluss::Series& d,
    double sample_time = 0.0);
```

## What It Does

Validates and bundles `A`, `B`, `C` and `D` into a `StateSpace` for `x' = A x + B u`, `y = C x + D u` (continuous) or `x[k+1] = A x[k] + B u[k]` (discrete). The matrices carry their last two axes as the matrix and any leading axes as a batch of systems that share one sample time.

## Parameters

`a`
: `[..., n, n]`, the dynamics. Floating or complex.

`b`
: `[..., n, m]`, the input matrix.

`c`
: `[..., p, n]`, the output matrix.

`d`
: `[..., p, m]`, the feedthrough.

`sample_time`
: Zero (the default) for a continuous system, a positive number of seconds for a discrete one.

## Returns

A `StateSpace` with `n` states, `m` inputs and `p` outputs.

## Errors

A non-square `A`, a `B`, `C` or `D` whose shape does not match, a rank below two, an integer dtype and a negative or non-finite sample time throw `pluss::Error`.

## Interpretation and Use

The sizes are read from the matrices: the first example shows a one-input, one-output oscillator and a three-state, two-input, two-output system. A leading axis holds a family of systems (the third example builds four oscillators with different damping) so that one call analyses or simulates them all.

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The state, input and output counts of a single-input and a multi-input system.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace siso = oscillator();
  out.writeln("oscillator: states", siso.states(), "inputs", siso.inputs(), "outputs", siso.outputs(), " continuous:", siso.is_cont());
  const lti::StateSpace mimo = lti::state(pluss::series::eye(3, pluss::ScalarType::Float64).mul(-1.0), pluss::series::ones({3, 2}, pluss::ScalarType::Float64),
                                         pluss::series::ones({2, 3}, pluss::ScalarType::Float64), pluss::series::zeros({2, 2}, pluss::ScalarType::Float64));
  out.writeln("three states, two inputs, two outputs: states", mimo.states(), "inputs", mimo.inputs(), "outputs", mimo.outputs());
}
```

Output

```text
oscillator: states 2 inputs 1 outputs 1  continuous: true
three states, two inputs, two outputs: states 3 inputs 2 outputs 2
```
### Example 2

A continuous and a discrete model: how the sample time is read back.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const lti::StateSpace continuous = lti::state(mat({-1.0}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1));
  const lti::StateSpace discrete = lti::state(mat({0.5}, 1, 1), mat({1.0}, 1, 1), mat({1.0}, 1, 1), mat({0.0}, 1, 1), 0.1);
  out.writeln("continuous: is_cont", continuous.is_cont(), "is_disc", continuous.is_disc(), "sample time", continuous.sample_time());
  out.writeln("discrete:   is_cont", discrete.is_cont(), "is_disc", discrete.is_disc(), "sample time", discrete.sample_time());
}
```

Output

```text
continuous: is_cont true is_disc false sample time 0.0000
discrete:   is_cont false is_disc true sample time 0.1000
```
### Example 3

A batch of four systems on a leading axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // A leading axis holds four systems that differ in their damping; the matrix axes stay last.
  std::vector<double> a;
  for (const double damping : {0.0, 0.2, 0.4, 0.8}) for (const double v : {0.0, 1.0, -4.0, -damping}) a.push_back(v);
  const lti::StateSpace batch = lti::state(pluss::make_series(a, {4, 2, 2}), pluss::series::zeros({4, 2, 1}, pluss::ScalarType::Float64).add(mat({0.0, 1.0}, 2, 1)),
                                          pluss::series::zeros({4, 1, 2}, pluss::ScalarType::Float64).add(mat({1.0, 0.0}, 1, 2)), pluss::series::zeros({4, 1, 1}, pluss::ScalarType::Float64));
  out.writeln("A shape", batch.a().size(0), "x", batch.a().size(1), "x", batch.a().size(2), " states", batch.states(), "inputs", batch.inputs(), "outputs", batch.outputs());
}
```

Output

```text
A shape 4 x 2 x 2  states 2 inputs 1 outputs 1
```
### Example 4

Every refused matrix and sample time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <string>

namespace lti = pluss::subsp::eng::lti;
using namespace pluss::tils;

template <class F> void refused(OutputEngine &out, const char *label, F &&call) {
  try {
    call();
    out.writeln(label, ": accepted");
  } catch (const pluss::Error &e) {
    out.writeln(label, ":", e.message());
  }
}

static pluss::Series mat(std::vector<double> values, std::int64_t rows, std::int64_t cols) { return pluss::make_series(std::move(values), {rows, cols}); }

// A damped oscillator x'' + 0.4 x' + 4 x = u with the position as output.
static lti::StateSpace oscillator() { return lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1)); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series a = mat({0.0, 1.0, -4.0, -0.4}, 2, 2), b = mat({0.0, 1.0}, 2, 1), c = mat({1.0, 0.0}, 1, 2), d = mat({0.0}, 1, 1);
  refused(out, "a non-square A", [&] { lti::state(mat({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, 2, 3), b, c, d); });
  refused(out, "B with the wrong number of rows", [&] { lti::state(a, mat({1.0, 2.0, 3.0}, 3, 1), c, d); });
  refused(out, "C with the wrong number of columns", [&] { lti::state(a, b, mat({1.0, 0.0, 0.0}, 1, 3), d); });
  refused(out, "D that does not match the outputs and inputs", [&] { lti::state(a, b, c, mat({0.0, 0.0}, 1, 2)); });
  refused(out, "a negative sample time", [&] { lti::state(a, b, c, d, -0.1); });
}
```

Output

```text
a non-square A : eng::lti::state: A must be square
B with the wrong number of rows : eng::lti::state: B rows must equal the state count
C with the wrong number of columns : eng::lti::state: C columns must equal the state count
D that does not match the outputs and inputs : eng::lti::state: D must have shape [outputs, inputs]
a negative sample time : eng::lti::state: sample_time must be finite and non-negative
```
## See Also

See the [State space](../state.md) family page for matrix shapes, batching, and analysis guidance.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Gram.html">
    <span class="label">Previous</span>
    <span class="title">Gram</span>
  </a>
  <a class="next-link" href="ctrb.html">
    <span class="label">Next</span>
    <span class="title">ctrb</span>
  </a>
</div>
<!-- apiref:nav:end -->
