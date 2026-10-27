// Complex-parameter training support, added on top of the Float64 pass in
// nn_optim_float64_test.cpp: real torch trains complex Parameters correctly
// in SGD/Adam/AdamW/RMSprop/NAdam/Adagrad (verified live against this
// repo's own libtorch build before touching any code -- see
// project_plussai_optim_float64_support memory). The one real fix needed
// was every optimizer's second-moment/variance accumulation, which used
// `g.square()` (= g^2, itself complex and NOT a magnitude) where it needs
// `g.abs2()` (= |g|^2, real-valued even for complex g) -- exactly the
// convention torch's own complex-Adam uses internally. Elementwise
// gradient clipping (ByValue, via clamp) correctly still rejects complex
// (matches libtorch's clip_grad_value_ failing the same way -- no natural
// ordering over complex numbers); norm-based clipping (ByNorm) needed the
// same square()->abs2() fix and then works, matching libtorch's
// clip_grad_norm_ supporting complex.
//
// Oracle values below come from real torch (verified live), but with an
// EXPLICIT gradient supplied directly (`p.grad = ...; opt.step()`) rather
// than via `.backward()` -- this sidesteps any question of whether Pluss's
// own complex-autodiff convention (conjugate vs. not) matches torch's,
// since it isn't exercised here: this file tests the OPTIMIZER's own math
// given a gradient, not autodiff itself (that's Track A/C's territory,
// already covered elsewhere for complex reverse-mode).
#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using Complex = std::complex<double>;

Series complex_series(const std::vector<Complex>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Complex128);
}

std::vector<Complex> vec_c128(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Complex128).host<Complex>();
}

bool approx_c(Complex a, Complex b, double tol = 1e-6) {
  return std::abs((a - b)) <= tol;
}

bool approx_vec_c(const std::vector<Complex>& a, const std::vector<Complex>& b, double tol = 1e-6) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!approx_c(a[i], b[i], tol)) return false;
  }
  return true;
}

const std::vector<Complex> kP0{{1.0, 2.0}, {-0.5, 0.5}};
const std::vector<Complex> kG0{{2.0, 4.0}, {-1.0, 1.0}};

bool check(const char* name, const std::vector<Complex>& expected, const Series& p, std::ostream* err) {
  if (p.scalar_type() != ScalarType::Complex128) {
    if (err) *err << name << ": step did not preserve Complex128 dtype\n";
    return false;
  }
  const auto got = vec_c128(p);
  if (!approx_vec_c(got, expected, 1e-6)) {
    if (err) {
      *err << name << ": mismatch, got={";
      for (auto v : got) *err << v << " ";
      *err << "} expected={";
      for (auto v : expected) *err << v << " ";
      *err << "}\n";
    }
    return false;
  }
  return true;
}

bool run_optim_complex_checks(std::ostream* err = nullptr) {
  const Config cfg = Config{}.with_dtype(ScalarType::Complex128).with_platform(pluss::pCPU);

  // SGD: p -= lr * g. No moment estimate, no clipping -- already
  // dtype-generic before this pass; included for completeness.
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 0.1;
    pluss::nn::optim::SGD sgd(std::vector<Series*>{&p}, opt);
    sgd.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("SGD", {{0.8, 1.6}, {-0.4, 0.4}}, p, err)) return false;
  }

  // Adam: m1=(1-b1)g, m2=(1-b2)|g|^2 (was g^2), bias-corrected at step 1,
  // p -= lr*m1_hat/(sqrt(m2_hat)+eps).
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 0.1;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.epsilon = 1e-8;
    pluss::nn::optim::Adam adam(std::vector<Series*>{&p}, opt);
    adam.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("Adam", {{0.9000000005, 1.90000000025}, {-0.400000001, 0.400000001}}, p, err)) return false;
  }

  // AdamW: same as Adam, plus decoupled weight decay (p *= 1 - lr*wd first).
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::AdamWOptions opt{};
    opt.learning_rate = 0.1;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.epsilon = 1e-8;
    opt.weight_decay = 0.01;
    pluss::nn::optim::AdamW adamw(std::vector<Series*>{&p}, opt);
    adamw.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("AdamW", {{0.8990000005, 1.89800000025}, {-0.399500001, 0.399500001}}, p, err)) return false;
  }

  // RMSprop: v=alpha*v+(1-alpha)|g|^2 (was g^2), p -= lr*g/(sqrt(v)+eps).
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::RMSpropOptions opt{};
    opt.learning_rate = 0.1;
    opt.alpha = 0.99;
    opt.epsilon = 1e-8;
    pluss::nn::optim::RMSprop rms(std::vector<Series*>{&p}, opt);
    rms.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("RMSprop", {{5e-08, 1.0000000249999998}, {0.4999999000000096, -0.4999999000000096}}, p, err)) {
      return false;
    }
  }

  // NAdam: Nesterov-accelerated Adam variant, same |g|^2 second-moment fix.
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::NAdamOptions opt{};
    opt.learning_rate = 0.1;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.epsilon = 1e-8;
    opt.momentum_decay = 0.004;
    pluss::nn::optim::NAdam nadam(std::vector<Series*>{&p}, opt);
    nadam.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("NAdam", {{0.8943548213947469, 1.8943548211306338}, {-0.39435482192297266, 0.39435482192297266}},
               p, err)) {
      return false;
    }
  }

  // Adagrad: s += |g|^2 (was g^2), p -= lr*g/(sqrt(s)+eps).
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::AdagradOptions opt{};
    opt.learning_rate = 0.1;
    opt.epsilon = 1e-8;
    pluss::nn::optim::Adagrad adagrad(std::vector<Series*>{&p}, opt);
    adagrad.step(std::vector<Series>{complex_series(kG0, {2})});
    if (!check("Adagrad", {{0.9000000005, 1.90000000025}, {-0.400000001, 0.400000001}}, p, err)) return false;
  }

  // Norm-based gradient clipping: reduces to a real, non-negative scalar
  // (matches libtorch's clip_grad_norm_ supporting complex). g=[2+4i,-1+i],
  // |g|=[sqrt(20),sqrt(2)], norm=sqrt(20+2)=sqrt(22)~=4.6904. threshold=1.0
  // => scale=1.0/(4.6904+eps)~=0.21320, clipped_g=g*scale.
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 1.0;
    opt.clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
    opt.clip.threshold = 1.0;
    pluss::nn::optim::SGD sgd(std::vector<Series*>{&p}, opt);
    sgd.step(std::vector<Series>{complex_series(kG0, {2})});
    const double scale = 1.0 / std::sqrt(22.0);
    const std::vector<Complex> expected{
        kP0[0] - kG0[0] * scale,
        kP0[1] - kG0[1] * scale,
    };
    if (!check("SGD norm-clip", expected, p, err)) return false;
  }

  // ByValue (elementwise clamp) clipping must still throw for complex --
  // no natural ordering, matches libtorch's clip_grad_value_ failing
  // identically ("clamp is not supported for complex types").
  {
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 1.0;
    opt.clip.mode = pluss::nn::optim::GradientClipMode::ByValue;
    opt.clip.threshold = 1.0;
    pluss::nn::optim::SGD sgd(std::vector<Series*>{&p}, opt);
    bool threw = false;
    try {
      sgd.step(std::vector<Series>{complex_series(kG0, {2})});
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "SGD value-clip: expected complex rejection, none occurred\n";
      return false;
    }
  }

  // Minimax's constrained-optimization machinery: box constraints and
  // inequality constraints are NOT the same kind of complex-ambiguous
  // question, so they're checked (and rejected/accepted) independently --
  // see the review that caught this distinction after the first pass
  // blanket-rejected all of Minimax's constraint machinery for any complex
  // parameter, which was imprecise (over-broad AND checking the wrong
  // dtype for the inequality case).
  {
    // Equality-only constraints never order anything (the multiplier update
    // is pure add/mul: eq_multipliers += residual * dual_lr) -- must work
    // for a complex parameter with no restriction at all.
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::MinimaxOptions opt{};
    opt.learning_rate = 0.05;
    opt.dual_learning_rate = 0.1;
    pluss::nn::optim::MinimaxConstraints constraints{};
    constraints.equalities = [](const std::vector<Series>& params) {
      return std::vector<Series>{params[0].sum()};
    };
    bool threw = false;
    try {
      pluss::nn::optim::Minimax minimax(std::vector<Series*>{&p}, opt, constraints);
      minimax.step(std::vector<Series>{complex_series(kG0, {2})});
    } catch (const std::exception& e) {
      threw = true;
      if (err) *err << "Minimax equality-only complex: unexpected throw: " << e.what() << "\n";
    }
    if (threw) return false;
  }

  {
    // A REAL-valued inequality residual on a COMPLEX parameter is
    // perfectly well-posed (h(x) = |x|^2 - 1 <= 0) -- the ordering the dual
    // multiplier needs is on the residual, not on the parameter itself, so
    // this must be accepted even though `p` is complex.
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::MinimaxOptions opt{};
    opt.learning_rate = 0.05;
    opt.dual_learning_rate = 0.1;
    pluss::nn::optim::MinimaxConstraints constraints{};
    constraints.inequalities = [](const std::vector<Series>& params) {
      return std::vector<Series>{params[0].abs2().sum().sub(10.0)};
    };
    bool threw = false;
    try {
      pluss::nn::optim::Minimax minimax(std::vector<Series*>{&p}, opt, constraints);
      minimax.step(std::vector<Series>{complex_series(kG0, {2})});
    } catch (const std::exception& e) {
      threw = true;
      if (err) *err << "Minimax real-residual-on-complex-param: unexpected throw: " << e.what() << "\n";
    }
    if (threw) return false;
  }

  {
    // A COMPLEX-valued inequality residual has no ordering convention
    // (relu()/clamp_min(0) on the dual multiplier's update has no meaning
    // over complex numbers) -- must still be rejected, regardless of
    // whether the parameter itself is real or complex.
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::MinimaxOptions opt{};
    opt.learning_rate = 0.05;
    opt.dual_learning_rate = 0.1;
    pluss::nn::optim::MinimaxConstraints constraints{};
    constraints.inequalities = [](const std::vector<Series>& params) {
      return std::vector<Series>{params[0].sum()};  // complex-valued residual
    };
    bool threw = false;
    try {
      pluss::nn::optim::Minimax minimax(std::vector<Series*>{&p}, opt, constraints);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Minimax complex-valued inequality residual: expected rejection, none occurred\n";
      return false;
    }
  }

  {
    // Box constraints order the PARAMETER's own value directly
    // (maximum(lower_bound)/minimum(upper_bound)) -- no ordering
    // convention over complex numbers exists for that either, and this one
    // genuinely is about the parameter's own dtype. Must still be rejected.
    Series p = complex_series(kP0, {2});
    pluss::nn::optim::MinimaxOptions opt{};
    opt.learning_rate = 0.05;
    pluss::nn::optim::MinimaxConstraints constraints{};
    constraints.lower_bounds = {complex_series({{0.0, 0.0}, {0.0, 0.0}}, {2})};
    bool threw = false;
    try {
      pluss::nn::optim::Minimax minimax(std::vector<Series*>{&p}, opt, constraints);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Minimax box constraint on complex parameter: expected rejection, none occurred\n";
      return false;
    }
  }

  (void)cfg;
  return true;
}

}  // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn optimizers train complex parameters matching libtorch's own conventions",
          "[nn][optim][complex]") {
  REQUIRE(run_optim_complex_checks(&std::cerr));
}
#else
int main() {
  return run_optim_complex_checks(&std::cerr) ? 0 : 1;
}
#endif
