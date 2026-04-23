#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using pluss::optim::AdamOptions;
using pluss::optim::AdamState;
using pluss::optim::Trace;
using pluss::optim::adam_state_like;
using pluss::optim::adam_step_inplace;
using pluss::optim::square;
using pluss::optim::sum;
using pluss::optim::value_and_grad;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series make_vec(const std::vector<double>& values, const Config& cfg) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

std::vector<double> to_vec_f64(const Series& x) {
  return x.to(pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

double to_scalar_f64(const Series& x) {
  return x.to(pCPU, ScalarType::Float64).item<double>();
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

struct AdamReferenceState {
  std::vector<double> m{};
  std::vector<double> v{};
  std::vector<double> max_v{};
  std::int64_t step{0};
};

void adam_reference_step(std::vector<double>& params,
                         const std::vector<double>& grad,
                         AdamReferenceState& state,
                         const AdamOptions& opt) {
  if (params.size() != grad.size()) {
    throw std::runtime_error("adam_reference_step size mismatch");
  }
  if (state.m.size() != params.size() || state.v.size() != params.size()) {
    state.m.assign(params.size(), 0.0);
    state.v.assign(params.size(), 0.0);
    state.max_v.assign(params.size(), 0.0);
    state.step = 0;
  }

  ++state.step;
  const double b1_corr = 1.0 - std::pow(opt.beta1, static_cast<double>(state.step));
  const double b2_corr = 1.0 - std::pow(opt.beta2, static_cast<double>(state.step));
  for (std::size_t i = 0; i < params.size(); ++i) {
    double p = params[i];
    double g = opt.maximize ? -grad[i] : grad[i];
    if (opt.weight_decay != 0.0) {
      if (opt.decoupled_weight_decay) {
        p *= (1.0 - opt.learning_rate * opt.weight_decay);
      } else {
        g += opt.weight_decay * p;
      }
    }
    state.m[i] = opt.beta1 * state.m[i] + (1.0 - opt.beta1) * g;
    state.v[i] = opt.beta2 * state.v[i] + (1.0 - opt.beta2) * g * g;
    const double mhat = state.m[i] / b1_corr;
    double vhat = state.v[i] / b2_corr;
    if (opt.amsgrad) {
      state.max_v[i] = std::max(state.max_v[i], state.v[i]);
      vhat = state.max_v[i] / b2_corr;
    }
    const double update = opt.learning_rate * (mhat / (std::sqrt(vhat) + opt.eps));
    params[i] = p - update;
  }
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  // 1) Exact update parity against independent reference (Torch-style exactness spirit).
  {
    std::vector<Series> params = {
        make_vec({1.5, -2.0, 0.25}, cfg),
        make_vec({-0.5, 3.0}, cfg),
    };
    AdamState state = adam_state_like(params);
    AdamOptions opt{};
    opt.learning_rate = 0.1;
    opt.beta1 = 0.9;
    opt.beta2 = 0.999;
    opt.eps = 1e-8;
    opt.weight_decay = 1e-2;

    std::vector<double> ref_params = {1.5, -2.0, 0.25, -0.5, 3.0};
    AdamReferenceState ref_state{};

    const std::vector<std::vector<double>> grad_seq = {
        {0.4, -0.1, 0.2, -0.3, 0.5},
        {0.2, 0.6, -0.8, 0.4, -0.2},
        {-0.5, 0.3, 0.1, -0.9, 0.7},
    };

    for (const auto& grad_flat : grad_seq) {
      const std::vector<Series> grads = {
          make_vec({grad_flat[0], grad_flat[1], grad_flat[2]}, cfg),
          make_vec({grad_flat[3], grad_flat[4]}, cfg),
      };
      adam_step_inplace(params, grads, state, opt);
      adam_reference_step(ref_params, grad_flat, ref_state, opt);
    }

    const std::vector<double> got = {
        to_vec_f64(params[0])[0],
        to_vec_f64(params[0])[1],
        to_vec_f64(params[0])[2],
        to_vec_f64(params[1])[0],
        to_vec_f64(params[1])[1],
    };
    expect_vec_near(got, ref_params, 1e-12, "adam exact update parity", &failures);
    expect(state.step == ref_state.step, "adam step counter parity", &failures);
  }

  // 2) Weight decay-only contraction when gradients are zero.
  {
    std::vector<Series> params = {make_vec({2.0, -3.0}, cfg)};
    AdamState state = adam_state_like(params);
    AdamOptions opt{};
    opt.learning_rate = 0.05;
    opt.weight_decay = 0.2;

    const std::vector<Series> zero_grads = {make_vec({0.0, 0.0}, cfg)};
    adam_step_inplace(params, zero_grads, state, opt);
    const auto p = to_vec_f64(params[0]);
    expect(std::abs(p[0]) < 2.0 && std::abs(p[1]) < 3.0, "adam weight decay should contract params", &failures);
  }

  // 3) Deterministic replay and state continuity.
  {
    const AdamOptions opt{};
    std::vector<Series> a = {make_vec({-0.8, 1.2, -2.4}, cfg)};
    std::vector<Series> b = {make_vec({-0.8, 1.2, -2.4}, cfg)};
    AdamState sa = adam_state_like(a);
    AdamState sb = adam_state_like(b);

    std::vector<std::vector<double>> seq;
    seq.reserve(60);
    for (std::int64_t t = 0; t < 60; ++t) {
      seq.push_back({
          std::sin(0.1 * static_cast<double>(t + 1)),
          std::cos(0.2 * static_cast<double>(t + 1)),
          std::sin(0.3 * static_cast<double>(t + 1))});
    }

    for (std::size_t i = 0; i < seq.size(); ++i) {
      const std::vector<Series> g = {make_vec(seq[i], cfg)};
      adam_step_inplace(a, g, sa, opt);
      if (i < 20) adam_step_inplace(b, g, sb, opt);
    }
    for (std::size_t i = 20; i < seq.size(); ++i) {
      const std::vector<Series> g = {make_vec(seq[i], cfg)};
      adam_step_inplace(b, g, sb, opt);
    }

    expect_vec_near(to_vec_f64(a[0]), to_vec_f64(b[0]), 1e-12, "adam deterministic replay", &failures);
    expect(sa.step == sb.step, "adam state continuity step count", &failures);
  }

  // 4) Error path contracts (shape mismatch, arity mismatch).
  {
    bool threw_shape = false;
    try {
      std::vector<Series> params = {make_vec({1.0, 2.0}, cfg)};
      std::vector<Series> bad_grads = {make_vec({1.0, 2.0, 3.0}, cfg)};
      AdamState st = adam_state_like(params);
      adam_step_inplace(params, bad_grads, st);
    } catch (const std::exception&) {
      threw_shape = true;
    }
    expect(threw_shape, "adam should throw on gradient shape mismatch", &failures);

    bool threw_arity = false;
    try {
      std::vector<Series> params = {make_vec({1.0, 2.0}, cfg)};
      std::vector<Series> bad_grads = {make_vec({1.0, 2.0}, cfg), make_vec({3.0}, cfg)};
      AdamState st = adam_state_like(params);
      adam_step_inplace(params, bad_grads, st);
    } catch (const std::exception&) {
      threw_arity = true;
    }
    expect(threw_arity, "adam should throw on params/grad arity mismatch", &failures);
  }

  // 5) Convergence-style stress: repeated gradient updates on a non-linear objective.
  {
    std::vector<Series> params = {make_vec({4.0, -3.5}, cfg)};
    AdamState st = adam_state_like(params);
    AdamOptions opt{};
    opt.learning_rate = 0.03;
    opt.weight_decay = 1e-4;

    double start_obj = std::numeric_limits<double>::infinity();
    double end_obj = std::numeric_limits<double>::infinity();
    for (std::int64_t iter = 0; iter < 1500; ++iter) {
      const auto [value, grads] = value_and_grad(
          [](const std::vector<Trace>& xs) -> Trace {
            const Trace x = xs[0];
            return sum(square(square(x) - 1.0));
          },
          params);
      if (iter == 0) start_obj = to_scalar_f64(value);
      end_obj = to_scalar_f64(value);
      adam_step_inplace(params, grads, st, opt);
    }
    expect(std::isfinite(end_obj), "adam stress objective should stay finite", &failures);
    expect(end_obj < 1e-6, "adam should converge on non-linear quartic objective", &failures);
    expect(end_obj < start_obj, "adam should decrease objective over training", &failures);
  }

  // 6) AMSGrad and maximize parity against independent reference.
  {
    std::vector<Series> params = {make_vec({1.25, -0.75}, cfg)};
    AdamState state = adam_state_like(params);
    AdamOptions opt{};
    opt.learning_rate = 0.05;
    opt.beta1 = 0.8;
    opt.beta2 = 0.97;
    opt.eps = 1e-9;
    opt.weight_decay = 1e-2;
    opt.amsgrad = true;
    opt.maximize = true;

    std::vector<double> ref_params = {1.25, -0.75};
    AdamReferenceState ref_state{};

    const std::vector<std::vector<double>> grad_seq = {
        {0.2, -0.1},
        {-0.3, 0.4},
        {0.7, -0.2},
        {-0.6, -0.5},
    };
    for (const auto& grad_flat : grad_seq) {
      const std::vector<Series> grads = {make_vec(grad_flat, cfg)};
      adam_step_inplace(params, grads, state, opt);
      adam_reference_step(ref_params, grad_flat, ref_state, opt);
    }
    expect_vec_near(to_vec_f64(params[0]), ref_params, 1e-12, "adam amsgrad+maximize parity", &failures);
  }

  // 7) Decoupled weight decay parity (AdamW-style decay behavior).
  {
    std::vector<Series> params = {make_vec({2.0, -1.0}, cfg)};
    AdamState state = adam_state_like(params);
    AdamOptions opt{};
    opt.learning_rate = 0.03;
    opt.weight_decay = 0.2;
    opt.decoupled_weight_decay = true;

    std::vector<double> ref_params = {2.0, -1.0};
    AdamReferenceState ref_state{};
    const std::vector<std::vector<double>> grad_seq = {
        {0.0, 0.0},
        {0.1, -0.2},
        {-0.05, 0.3},
    };
    for (const auto& grad_flat : grad_seq) {
      const std::vector<Series> grads = {make_vec(grad_flat, cfg)};
      adam_step_inplace(params, grads, state, opt);
      adam_reference_step(ref_params, grad_flat, ref_state, opt);
    }
    expect_vec_near(to_vec_f64(params[0]), ref_params, 1e-12, "adam decoupled weight decay parity", &failures);
  }

  if (failures != 0) {
    std::cerr << "adam_functional_torch_rigor_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "adam_functional_torch_rigor_test passed\n";
  return 0;
}
