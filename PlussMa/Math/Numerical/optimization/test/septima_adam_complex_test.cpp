// Complex-parameter support for pluss::optim::adam_step_inplace (the Adam
// half of Septima's SeptimaOptions::Optimizer::Adam branch), mirroring the
// fix + verification already done for PlussAI/NodeK's Adam/AdamW/RMSprop/
// NAdam/Adagrad and Minimax (see project_plussai_optim_float64_support
// memory). Two real, distinct bugs existed here before this fix, both
// found by re-reading this file after the user asked whether Septima and
// Minimax recover from NaN the way Optima does:
//
//   1. `adam_step_inplace`'s second-moment accumulator used `g.mul(g)`
//      (= g^2, itself complex) instead of tracking the real and imaginary
//      channels independently -- NOT the same quantity as torch's own
//      complex-Adam convention (real division mixes real/imaginary parts,
//      so no single intermediate fix works; the whole formula has to run
//      twice, once per channel -- see optim.h's detail::view_as_real_ for
//      the full derivation, identical to optim_base.h's).
//   2. `detail::septima_finite_` called `Series::isfinite()` directly,
//      which has no complex-dtype implementation in CodeM at all -- so
//      ANY complex-parameter Septima Adam run threw immediately
//      ("predicate unary: dtype not implemented"), since septima_finite_
//      is called unconditionally on every attempt of the NaN-recovery
//      loop in septima_step_inplace_, not just when something has
//      actually gone non-finite.
//
// Verification strategy: rather than hand-deriving AMSGrad's formula
// through several steps by hand, exploit the DEFINING property of the
// view-as-real fix directly -- running Adam on one complex parameter must
// give bit-for-bit (well, float-tolerance) the same trajectory as running
// two ordinary REAL Adam optimizers side by side, one on the real part and
// one on the imaginary part, with the same per-step gradients split the
// same way. That is a strictly stronger, oracle-free check than any single
// hand-copied numeric example, and it exercises multi-step state (m, v,
// max_v) drift, not just a single step.
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using Complex = std::complex<double>;

using pluss::optim::AdamOptions;
using pluss::optim::AdamState;
using pluss::optim::adam_state_like;
using pluss::optim::adam_step_inplace;
using pluss::optim::septima;
using pluss::optim::SeptimaOptions;
using pluss::optim::SeptimaSolveOptions;
using pluss::optim::SeptimaExitFlag;
using pluss::optim::Trace;
using pluss::optim::sum;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series complex_series(const std::vector<Complex>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Complex128);
}

Series real_series(const std::vector<double>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Float64);
}

std::vector<double> to_vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

std::vector<Complex> to_vec_c128(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Complex128).reshape({x.numel()}).to_vec<Complex>();
}

void expect_vec_near(const std::vector<double>& lhs,
                     const std::vector<double>& rhs,
                     double atol,
                     const std::string& msg,
                     int* failures) {
  if (lhs.size() != rhs.size()) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " (size mismatch)\n";
    return;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (std::abs(lhs[i] - rhs[i]) > atol) {
      ++(*failures);
      std::cerr << "[FAIL] " << msg << " idx=" << i << " lhs=" << lhs[i] << " rhs=" << rhs[i] << '\n';
      return;
    }
  }
}

} // namespace

int main() {
  int failures = 0;

  // 1) Real-dtype regression: the view_as_real_/view_as_complex_ wiring
  // must be an exact no-op for a real parameter. Hand-derived: p=1.0,
  // g=0.5, lr=0.1, defaults (beta1=0.9, beta2=0.999, eps=1e-8) -> at
  // step 1 bias correction makes m1_hat=g, m2_hat=g^2, so
  // next = p - lr*g/(|g|+eps) ~= 1.0 - 0.1*1.0 = 0.9, up to the ~2e-9
  // deviation the eps=1e-8 term itself introduces (0.5/(0.5+1e-8) != 1
  // exactly) -- same order-of-magnitude deviation as the identical
  // hand-derived value already pinned for NodeK's Adam in
  // PlussAI/test/nn_optim_test.cpp's "fp64 param supported" case.
  {
    Series p = real_series({1.0}, {1});
    Series g = real_series({0.5}, {1});
    AdamState state = adam_state_like({p});
    AdamOptions opt{};
    opt.learning_rate = 0.1;
    std::vector<Series> params{p};
    adam_step_inplace(params, {g}, state, opt);
    expect_vec_near(to_vec_f64(params[0]), {0.9}, 1e-8,
                    "real-dtype Adam step regression (view-as-real must be a no-op)", &failures);
  }

  // 2) Complex-vs-split-real self-consistency: running Adam on a complex
  // parameter must exactly match running two independent real Adams, one
  // per channel, over several steps -- including amsgrad and (decoupled)
  // weight_decay, so the max_v ordering op and the weight-decay-before-
  // view-as-real ordering are both exercised.
  {
    const std::vector<Complex> p0{{1.0, 2.0}, {-0.5, 0.5}};
    // A short, arbitrary per-step gradient sequence (not all equal, so
    // this actually exercises moment-buffer drift across steps).
    const std::vector<std::vector<Complex>> g_steps{
        {{2.0, 4.0}, {-1.0, 1.0}},
        {{0.5, -1.0}, {2.0, -0.5}},
        {{-1.5, 0.25}, {0.75, 2.0}},
        {{1.0, 1.0}, {-2.0, -1.0}},
        {{0.1, -0.1}, {0.3, 0.3}},
    };

    auto split_channel = [&](std::size_t which) {
      std::vector<std::vector<double>> out;
      for (const auto& g : g_steps) {
        std::vector<double> row;
        for (const auto& c : g) row.push_back(which == 0 ? c.real() : c.imag());
        out.push_back(row);
      }
      return out;
    };
    const auto g_re_steps = split_channel(0);
    const auto g_im_steps = split_channel(1);

    std::vector<double> p0_re, p0_im;
    for (const auto& c : p0) {
      p0_re.push_back(c.real());
      p0_im.push_back(c.imag());
    }

    AdamOptions opt{};
    opt.learning_rate = 0.05;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.eps = 1e-8;
    opt.amsgrad = true;
    opt.weight_decay = 0.01;
    opt.decoupled_weight_decay = true;

    Series p_complex = complex_series(p0, {2});
    AdamState state_complex = adam_state_like({p_complex});
    std::vector<Series> params_complex{p_complex};

    Series p_re = real_series(p0_re, {2});
    Series p_im = real_series(p0_im, {2});
    AdamState state_re = adam_state_like({p_re});
    AdamState state_im = adam_state_like({p_im});
    std::vector<Series> params_re{p_re};
    std::vector<Series> params_im{p_im};

    for (std::size_t step = 0; step < g_steps.size(); ++step) {
      adam_step_inplace(params_complex, {complex_series(g_steps[step], {2})}, state_complex, opt);
      adam_step_inplace(params_re, {real_series(g_re_steps[step], {2})}, state_re, opt);
      adam_step_inplace(params_im, {real_series(g_im_steps[step], {2})}, state_im, opt);
    }

    const auto got = to_vec_c128(params_complex[0]);
    const auto want_re = to_vec_f64(params_re[0]);
    const auto want_im = to_vec_f64(params_im[0]);
    std::vector<double> got_re, got_im;
    for (const auto& c : got) {
      got_re.push_back(c.real());
      got_im.push_back(c.imag());
    }
    expect_vec_near(got_re, want_re, 1e-9,
                    "complex Adam real channel must match split-real Adam (amsgrad+weight_decay)", &failures);
    expect_vec_near(got_im, want_im, 1e-9,
                    "complex Adam imag channel must match split-real Adam (amsgrad+weight_decay)", &failures);
    if (params_complex[0].scalar_type() != ScalarType::Complex128) {
      ++failures;
      std::cerr << "[FAIL] complex Adam step did not preserve Complex128 dtype\n";
    }
  }

  // 3) Same oracle values already verified live against real torch for
  // PlussAI/NodeK's Adam (project_plussai_optim_float64_support memory) --
  // Septima's adam_step_inplace implements the identical non-capturable,
  // non-clipped formula shape, so the same single-step numbers must hold
  // here too.
  {
    Series p = complex_series({{1.0, 2.0}, {-0.5, 0.5}}, {2});
    Series g = complex_series({{2.0, 4.0}, {-1.0, 1.0}}, {2});
    AdamState state = adam_state_like({p});
    AdamOptions opt{};
    opt.learning_rate = 0.1;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.eps = 1e-8;
    std::vector<Series> params{p};
    adam_step_inplace(params, {g}, state, opt);
    const auto got = to_vec_c128(params[0]);
    const std::vector<Complex> want{{0.9000000005, 1.90000000025}, {-0.400000001, 0.400000001}};
    for (std::size_t i = 0; i < got.size(); ++i) {
      if (std::abs(got[i] - want[i]) > 1e-6) {
        ++failures;
        std::cerr << "[FAIL] Septima Adam complex oracle mismatch idx=" << i << " got=" << got[i]
                  << " want=" << want[i] << '\n';
      }
    }
  }

  // 4) End-to-end product-path proof: septima_finite_ (the function that
  // was throwing "predicate unary: dtype not implemented" on ANY complex
  // Adam step before this fix, since it runs unconditionally on every
  // attempt of the NaN-recovery loop) must not throw across a real,
  // multi-iteration solve -- not just when directly unit-tested. Minimizes
  // sum(|x|^2) (a real-valued loss of a complex variable, well-posed and
  // already-verified complex-autodiff territory) from a nonzero starting
  // point; the parameter must move toward the origin.
  {
    Series x0 = complex_series({{1.0, 2.0}, {-0.5, 0.5}}, {2});

    SeptimaSolveOptions solve_opts{};
    solve_opts.step.optimizer = SeptimaOptions::Optimizer::Adam;
    solve_opts.step.adam.learning_rate = 0.1;
    solve_opts.max_iterations = 50;

    bool threw = false;
    SeptimaExitFlag exit_flag = SeptimaExitFlag::MaxIterations;
    double initial_norm = 0.0;
    double final_norm = 0.0;
    try {
      const auto fit = septima(
          [](const Trace& x) -> Trace {
            return sum(pluss::subsp::math::diff::pow(pluss::subsp::math::diff::abs(x), 2.0));
          },
          x0,
          solve_opts);
      exit_flag = fit.exit_flag;
      const auto x_final = to_vec_c128(fit.params.at(0));
      for (const auto& c : x_final) final_norm += std::abs(c) * std::abs(c);
      const auto x_initial = to_vec_c128(x0);
      for (const auto& c : x_initial) initial_norm += std::abs(c) * std::abs(c);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] septima() threw on a complex Adam solve: " << e.what() << '\n';
    }
    expect(!threw, "septima() must not throw on a complex-parameter Adam solve", &failures);
    if (!threw) {
      expect(exit_flag != SeptimaExitFlag::NonFiniteEvaluation,
             "septima() must not report NonFiniteEvaluation for a well-posed complex solve", &failures);
      expect(final_norm < initial_norm,
             "septima() must make progress minimizing sum(|x|^2) from a complex start", &failures);
    }
  }

  if (failures != 0) {
    std::cerr << "septima_adam_complex_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "septima_adam_complex_test passed\n";
  return 0;
}
