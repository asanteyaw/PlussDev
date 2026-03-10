#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-9) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!approx(a[i], b[i], tol)) return false;
  }
  return true;
}

struct ScalarModule final : pluss::nn::Module {
  Series weight;

  explicit ScalarModule(const Config& cfg)
      : Module(true), weight(pluss::make_series(1.0, cfg)) {
    parameter_record("weight", weight);
  }
};

bool run_nn_optimizer_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();

  Series sgd_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::SGDOptions sgd_opt{};
  sgd_opt.learning_rate = 0.1;
  pluss::nn::optim::SGD sgd(std::vector<Series*>{&sgd_param}, sgd_opt);
  sgd.step(std::vector<Series>{pluss::make_series(0.5, cfg)});
  if (!approx(scalar_f64(sgd_param), 0.95)) {
    if (err) *err << "SGD explicit sensitivity step mismatch\n";
    return false;
  }

  Series clip_param = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}, cfg);
  pluss::nn::optim::SGDOptions clip_opt{};
  clip_opt.learning_rate = 1.0;
  clip_opt.clip.mode = pluss::nn::optim::GradientClipMode::ByNorm;
  clip_opt.clip.threshold = 1.0;
  pluss::nn::optim::SGD sgd_clip(std::vector<Series*>{&clip_param}, clip_opt);
  sgd_clip.step(std::vector<Series>{pluss::make_series(std::vector<double>{3.0, 4.0}, {2}, cfg)});
  if (!approx_vec(vec_f64(clip_param), std::vector<double>{0.4, 0.2}, 1e-8)) {
    if (err) *err << "SGD norm clipping mismatch\n";
    return false;
  }

  Series adam_param = pluss::make_series(1.0, cfg);
  pluss::nn::optim::AdamOptions adam_opt{};
  adam_opt.learning_rate = 0.1;
  pluss::nn::optim::Adam adam(std::vector<Series*>{&adam_param}, adam_opt);
  const Series adam_sensitivity = pluss::make_series(0.5, cfg);
  adam.step(std::vector<Series>{adam_sensitivity});
  adam.step(std::vector<Series>{adam_sensitivity});
  if (!approx(scalar_f64(adam_param), 0.8, 1e-8)) {
    if (err) *err << "Adam update mismatch\n";
    return false;
  }

  Series tape_param = pluss::make_series(2.0, cfg);
  auto tracked = pluss::subsp::math::diff::trace(tape_param, true);
  const Series tape_loss = tracked.value().square();
  tracked.clear_sensitivity();
  tape_loss.reverse();
  pluss::nn::optim::SGDOptions tape_opt{};
  tape_opt.learning_rate = 0.25;
  pluss::nn::optim::SGD tape_sgd(std::vector<Series*>{&tape_param}, tape_opt);
  tape_sgd.step();
  if (!approx(scalar_f64(tape_param), 1.0, 1e-8)) {
    if (err) *err << "Optimizer::step() sensitivity pull mismatch\n";
    return false;
  }
  if (!tape_param.tracks_slope() || tape_param.has_sensitivity()) {
    if (err) *err << "parameter slope/sensitivity post-step invariant mismatch\n";
    return false;
  }

  ScalarModule module(cfg);
  pluss::nn::optim::SGD mod_sgd(module, sgd_opt);
  mod_sgd.step(std::vector<Series>{pluss::make_series(0.1, cfg)});
  if (!approx(scalar_f64(module.weight), 0.99, 1e-9)) {
    if (err) *err << "Module-based optimizer constructor mismatch\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn optimizers update parameters with sensitivity semantics") {
  REQUIRE(run_nn_optimizer_checks());
}
#else
int main() {
  if (!run_nn_optimizer_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nn_optim_test passed\n";
  return 0;
}
#endif
