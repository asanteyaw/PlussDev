<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `lti::StateSpace` Introduction

Holds `A`, `B`, `C`, `D` and the sample time. The matrices keep the shapes they were built with; the counts of states, inputs and outputs are read from them. A `StateSpace` can only be created through `state`, which guarantees the shapes agree.

Whether the model is stable depends on the eigenvalues of `A` (left half-plane for a continuous system, inside the unit circle for a discrete one). For a two-state oscillator the sum of the eigenvalues is the trace, so a negative trace means the oscillation decays (the second example).

## Practical Examples

The following programs isolate this symbol's normal contract, the property that distinguishes it, and its validation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The four matrices and their shapes.

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
  const lti::StateSpace system = oscillator();
  out.writeln("A =", system.a().flatten().items<double>());
  out.writeln("B =", system.b().flatten().items<double>());
  out.writeln("C =", system.c().flatten().items<double>());
  out.writeln("D =", system.d().flatten().items<double>());
  out.writeln("shapes: A", system.a().size(0), "x", system.a().size(1), " B", system.b().size(0), "x", system.b().size(1), " C", system.c().size(0), "x", system.c().size(1), " D", system.d().size(0), "x", system.d().size(1));
}
```

Output

```text
A = [0.0000, 1.0000, -4.0000, -0.4000]
B = [0.0000, 1.0000]
C = [1.0000, 0.0000]
D = [0.0000]
shapes: A 2 x 2  B 2 x 1  C 1 x 2  D 1 x 1
```
### Example 2

The trace of `A` for a negative, zero and positive damping.

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
  // The eigenvalues of A decide stability; with these four oscillators only the damped ones decay.
  for (const double damping : {-0.2, 0.0, 0.4}) {
    const lti::StateSpace s = lti::state(mat({0.0, 1.0, -4.0, -damping}, 2, 2), mat({0.0, 1.0}, 2, 1), mat({1.0, 0.0}, 1, 2), mat({0.0}, 1, 1));
    const auto trace = s.a().select(0, 0).items<double>()[0] + s.a().select(0, 1).items<double>()[1];
    out.writeln("damping", damping, ": trace of A =", trace, " (negative means the energy decays)");
  }
}
```

Output

```text
damping -0.2000 : trace of A = 0.2000  (negative means the energy decays)
damping 0.0000 : trace of A = 0.0000  (negative means the energy decays)
damping 0.4000 : trace of A = -0.4000  (negative means the energy decays)
```
### Example 3

Constructor errors seen through the type.

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
  const pluss::Series b = mat({0.0, 1.0}, 2, 1), c = mat({1.0, 0.0}, 1, 2), d = mat({0.0}, 1, 1);
  refused(out, "a rank-one A", [&] { lti::state(pluss::make_series({1.0, 2.0}), b, c, d); });
  refused(out, "an integer A", [&] { lti::state(pluss::make_series({1, 0, 0, 1}, {2, 2}), b, c, d); });
  refused(out, "a non-finite sample time", [&] { lti::state(mat({0.0, 1.0, -4.0, -0.4}, 2, 2), b, c, d, std::nan("")); });
}
```

Output

```text
a rank-one A : eng::lti::state: A must have at least two dimensions
an integer A : eng::lti::state: A must have a floating or complex dtype
a non-finite sample time : eng::lti::state: sample_time must be finite and non-negative
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="a.html">
    <span class="label">Next</span>
    <span class="title">a</span>
  </a>
</div>
<!-- apiref:nav:end -->
