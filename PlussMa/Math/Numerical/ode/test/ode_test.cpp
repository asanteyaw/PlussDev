// pluss::subsp::math::ode -- Fluxa (fluxa/fluxa_adjoint).
//
// Verification strategy:
//  - Forward accuracy checked against a hand-derived ANALYTIC closed-form
//    solution (exponential decay: dy/dt = -k*y; the harmonic oscillator:
//    d^2x/dt^2 = -omega^2*x), not just "runs without throwing".
//  - Adjoint gradients checked TWICE, independently: against the SAME
//    hand-derived analytic closed form (exponential decay has an exact,
//    checkable dy(t1)/dy0 and dy(t1)/dk), AND against direct autodiff
//    through the identical unrolled RK4 discretization (Pluss's own
//    already-trusted `.reverse()` engine used as an oracle for the
//    SPECIFIC discretization fluxa() itself computes, not the true
//    continuous ODE solution) -- if the custom step-wise adjoint
//    implementation had a bug, these two independent checks would very
//    likely disagree with each other even if one of them happened to
//    coincidentally look plausible.
//
// CORRECTED, after later investigation (see PlussQit task queue / project
// memory): an earlier version of this test built a vector-valued
// right-hand side via `y.select(dim, i)` decomposition then
// `.pile()`-recomposition, which threw `Series::view(): requires
// contiguous` a few RK4 steps in and was originally misdiagnosed as a
// genuine CodeM contiguity-tracking bug. It was not: `.pile()` STACKS
// along a NEW leading dimension (like torch.stack), not concatenate along
// an existing one (that's `.unite()`) -- piling two already-`[1]`-shaped
// pieces along dim 0 produces `[2, 1]`, not the intended `[2]`. Adding
// that extra dimension into the RK4 loop's arithmetic silently broadcasts
// (`[2] + [2,1] -> [2,2]`) instead of erroring immediately, and the
// resulting shape creep is what a later `.select()` + `.reshape()` in the
// loop legitimately failed on -- a real numel mismatch, correctly
// rejected. The only actual bug was that `Series::view()`'s error message
// collapsed "not contiguous" and "wrong numel" into one string
// ("requires contiguous"), which is what caused the misdiagnosis; that
// message has since been split to report which condition actually failed
// (see `Series::view()` in OpsInternal.cpp). The harmonic-oscillator test
// below still uses `A.matmul(y)` (a constant matrix times the state
// vector) instead of select+pile to build the RHS -- not because select+
// pile is broken, but because it is both correct and a more natural way
// to write a linear ODE's RHS.
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::diff::trace;
namespace ode = pluss::subsp::math::ode;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

void expect_near(double got, double want, double atol, const std::string& msg, int* failures) {
  if (std::abs(got - want) > atol) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " got=" << got << " want=" << want << " atol=" << atol << '\n';
  }
}

Series real_series(const std::vector<double>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Float64);
}

double scalar_f64(const Series& x) { return x.to(pluss::pCPU, ScalarType::Float64).item<double>(); }

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

} // namespace

int main() {
  int failures = 0;

  // ---------------------------------------------------------------
  // fluxa(): forward accuracy, exponential decay dy/dt = -k*y,
  // y(t)=y0*exp(-k*t).
  // ---------------------------------------------------------------
  const double y0v = 1.0, kv = 0.5, t0 = 0.0, t1 = 2.0;
  ode::OdeFn decay = [](double, const Series& y, const Series& theta) { return y.mul(theta.mul(-1.0)); };
  const Series y0 = real_series({y0v}, {1});
  const Series theta0 = real_series({kv}, {1});

  ode::OdeOptions opts;
  opts.steps = 1000;
  {
    const auto fwd = ode::fluxa(decay, y0, t0, t1, theta0, opts);
    const double want = y0v * std::exp(-kv * (t1 - t0));
    expect_near(scalar_f64(fwd.y_final), want, 1e-9, "fluxa forward matches analytic exponential decay", &failures);
    expect(fwd.converged, "fluxa reports converged", &failures);
    expect(fwd.steps == opts.steps, "fluxa reports the configured step count", &failures);
  }

  // ---------------------------------------------------------------
  // fluxa_adjoint(): gradients, checked against BOTH the analytic closed
  // form AND direct autodiff through the same discretization.
  // ---------------------------------------------------------------
  {
    const Series seed = real_series({1.0}, {1});
    const auto adj = ode::fluxa_adjoint(decay, y0, t0, t1, theta0, seed, opts);

    const double want_y_final = y0v * std::exp(-kv * (t1 - t0));
    const double want_grad_y0 = std::exp(-kv * (t1 - t0));
    const double want_grad_theta = -y0v * (t1 - t0) * std::exp(-kv * (t1 - t0));

    expect_near(scalar_f64(adj.y_final), want_y_final, 1e-9, "fluxa_adjoint forward value matches analytic solution",
               &failures);
    expect_near(scalar_f64(adj.grad_y0), want_grad_y0, 1e-6, "fluxa_adjoint grad_y0 matches analytic closed form",
               &failures);
    expect_near(scalar_f64(adj.grad_theta), want_grad_theta, 1e-6,
               "fluxa_adjoint grad_theta matches analytic closed form", &failures);
    expect(adj.forward_steps == opts.steps && adj.backward_steps == opts.steps,
           "fluxa_adjoint reports matching forward/backward step counts", &failures);

    // Independent oracle: direct autodiff through the IDENTICAL unrolled
    // RK4 discretization, using Series' own already-trusted .reverse()
    // over the whole trajectory at once (the naive, memory-heavy approach
    // the adjoint method exists to avoid) -- both should agree to near
    // machine precision, since they differentiate the exact same
    // computation, just via two structurally different code paths.
    auto ty0 = trace(y0, true);
    auto ttheta = trace(theta0, true);
    Series y = ty0.value();
    Series theta = ttheta.value();
    const double h = (t1 - t0) / static_cast<double>(opts.steps);
    double t = t0;
    for (std::int64_t i = 0; i < opts.steps; ++i) {
      const Series k1 = decay(t, y, theta);
      const Series k2 = decay(t + h * 0.5, y.add(k1.mul(h * 0.5)), theta);
      const Series k3 = decay(t + h * 0.5, y.add(k2.mul(h * 0.5)), theta);
      const Series k4 = decay(t + h, y.add(k3.mul(h)), theta);
      y = y.add(k1.add(k2.mul(2.0)).add(k3.mul(2.0)).add(k4).mul(h / 6.0));
      t += h;
    }
    y.reverse();
    expect_near(scalar_f64(y0.sensitivity()), scalar_f64(adj.grad_y0), 1e-10,
               "fluxa_adjoint grad_y0 matches direct autodiff through the same RK4 discretization", &failures);
    expect_near(scalar_f64(theta0.sensitivity()), scalar_f64(adj.grad_theta), 1e-10,
               "fluxa_adjoint grad_theta matches direct autodiff through the same RK4 discretization", &failures);
  }

  // ---------------------------------------------------------------
  // fluxa(): vector-valued state, the harmonic oscillator
  // d/dt[x,v] = A*[x,v], A = [[0,1],[-omega^2,0]].
  // x(0)=1, v(0)=0 -> x(t)=cos(omega*t), v(t)=-omega*sin(omega*t).
  // ---------------------------------------------------------------
  {
    const double omega = 1.3;
    const Series A = real_series({0.0, 1.0, -omega * omega, 0.0}, {2, 2});
    ode::OdeFn oscillator = [&A](double, const Series& y, const Series&) {
      return A.matmul(y.reshape({2, 1})).reshape({2});
    };
    const Series y0_vec = real_series({1.0, 0.0}, {2});
    ode::OdeOptions vec_opts;
    vec_opts.steps = 2000;
    const double tf = 1.7;
    const auto fwd = ode::fluxa(oscillator, y0_vec, 0.0, tf, pluss::series::undefined(), vec_opts);
    const auto v = vec_f64(fwd.y_final);
    expect_near(v[0], std::cos(omega * tf), 1e-6, "fluxa harmonic oscillator x(t1) matches analytic solution",
               &failures);
    expect_near(v[1], -omega * std::sin(omega * tf), 1e-6,
               "fluxa harmonic oscillator v(t1) matches analytic solution", &failures);
  }

  // ---------------------------------------------------------------
  // fluxa_adjoint(): vector-valued gradient -- d(x(t1))/d(y0) for the
  // harmonic oscillator has a known closed form too: x(t1) is linear in
  // [x0,v0] via the state-transition matrix exp(A*t1) = [[cos(wt),
  // sin(wt)/w], [-w*sin(wt), cos(wt)]], so d(x(t1))/dx0 = cos(w*t1) and
  // d(x(t1))/dv0 = sin(w*t1)/w.
  // ---------------------------------------------------------------
  {
    const double omega = 1.3;
    const Series A = real_series({0.0, 1.0, -omega * omega, 0.0}, {2, 2});
    ode::OdeFn oscillator = [&A](double, const Series& y, const Series&) {
      return A.matmul(y.reshape({2, 1})).reshape({2});
    };
    const Series y0_vec = real_series({1.0, 0.0}, {2});
    ode::OdeOptions vec_opts;
    vec_opts.steps = 2000;
    const double tf = 1.7;
    // Seed = d(loss)/dy(t1) for loss = x(t1) alone: [1, 0].
    const Series seed = real_series({1.0, 0.0}, {2});
    const auto adj = ode::fluxa_adjoint(oscillator, y0_vec, 0.0, tf, pluss::series::undefined(), seed, vec_opts);
    const auto grad = vec_f64(adj.grad_y0);
    expect_near(grad[0], std::cos(omega * tf), 1e-5, "fluxa_adjoint d(x(t1))/dx0 matches state-transition closed form",
               &failures);
    expect_near(grad[1], std::sin(omega * tf) / omega, 1e-5,
               "fluxa_adjoint d(x(t1))/dv0 matches state-transition closed form", &failures);
  }

  if (failures != 0) {
    std::cerr << "ode_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "ode_test passed\n";
  return 0;
}
