#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::nn::optim::UpdateLane;

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    const char* old = std::getenv(key);
    if (old != nullptr) {
      had_old_ = true;
      old_ = old;
    }
    setenv(key, value, 1);
  }

  ~ScopedEnvVar() {
    if (had_old_) {
      setenv(key_.c_str(), old_.c_str(), 1);
    } else {
      unsetenv(key_.c_str());
    }
  }

private:
  std::string key_;
  std::string old_;
  bool had_old_{false};
};

Config train_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

Config train_mps_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-6) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!approx(a[i], b[i], tol)) return false;
  }
  return true;
}

template <class OptimizerT, class OptionsT>
bool run_foreach_optimizer_case_(const char* name,
                                 const OptionsT& base_options,
                                 int steps,
                                 const Config& cfg,
                                 std::ostream* err) {
  Series p_batched = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
  Series p_foreach = p_batched.clone();
  const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);

  OptionsT bat_opt = base_options;
  bat_opt.execution.lane = UpdateLane::Batched;
  OptionsT fe_opt = base_options;
  fe_opt.execution.lane = UpdateLane::Foreach;
  fe_opt.execution.strict = true;

  OptimizerT batched(std::vector<Series*>{&p_batched}, bat_opt);
  OptimizerT foreach(std::vector<Series*>{&p_foreach}, fe_opt);

  for (int i = 0; i < steps; ++i) {
    batched.step(std::vector<Series>{g});
    foreach.step(std::vector<Series>{g});
  }

  if (!approx_vec(vec_f64(p_batched), vec_f64(p_foreach), 1e-6)) {
    if (err) *err << name << ": foreach/batched parity mismatch\n";
    return false;
  }

  const auto& lane = foreach.last_step_lane_telemetry();
  if (lane.size() != 1 ||
      lane.front().requested != UpdateLane::Foreach ||
      lane.front().resolved != UpdateLane::Foreach ||
      !lane.front().foreach_supported ||
      !lane.front().batched_supported) {
    if (err) *err << name << ": foreach lane telemetry mismatch\n";
    return false;
  }
  return true;
}

bool run_foreach_lane_checks(std::ostream* err = nullptr) {
  {
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 0.05;
    opt.momentum = 0.9;
    opt.nesterov = true;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::SGD>("SGD", opt, 8, train_cfg(), err)) return false;
  }
  {
    pluss::nn::optim::AdagradOptions opt{};
    opt.learning_rate = 0.05;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::Adagrad>("Adagrad", opt, 8, train_cfg(), err)) return false;
  }
  {
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-3;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::Adam>("Adam", opt, 8, train_cfg(), err)) return false;
  }
  {
    pluss::nn::optim::AdamWOptions opt{};
    opt.learning_rate = 1e-3;
    opt.weight_decay = 1e-2;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::AdamW>("AdamW", opt, 8, train_cfg(), err)) return false;
  }
  {
    pluss::nn::optim::NAdamOptions opt{};
    opt.learning_rate = 1e-3;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::NAdam>("NAdam", opt, 8, train_cfg(), err)) return false;
  }
  {
    pluss::nn::optim::RMSpropOptions opt{};
    opt.learning_rate = 1e-3;
    opt.momentum = 0.5;
    if (!run_foreach_optimizer_case_<pluss::nn::optim::RMSprop>("RMSprop", opt, 8, train_cfg(), err)) return false;
  }

  {
    const Config cfg = train_cfg();
    Series p0 = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
    Series p1 = pluss::make_series(std::vector<float>{0.5f, 1.0f, -1.5f}, {3}, cfg);
    const Series g0 = pluss::make_series(std::vector<float>{0.1f, -0.2f}, {2}, cfg);
    const Series g1 = pluss::make_series(std::vector<float>{0.05f, 0.1f, -0.15f}, {3}, cfg);

    pluss::nn::optim::SGDOptions strict_foreach{};
    strict_foreach.learning_rate = 0.1;
    strict_foreach.execution.lane = UpdateLane::Foreach;
    strict_foreach.execution.strict = true;

    bool threw = false;
    try {
      pluss::nn::optim::SGD tx(std::vector<Series*>{&p0, &p1}, strict_foreach);
      tx.step(std::vector<Series>{g0, g1});
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Foreach strict lane should fail on incompatible batch shapes\n";
      return false;
    }
  }

  return true;
}

template <class OptimizerT, class OptionsT>
bool run_mps_optimizer_lane_case_(const char* name,
                                  OptionsT base_options,
                                  int steps,
                                  std::ostream* err) {
  const Config cfg = train_mps_cfg();
  Series p_loop = pluss::make_series(std::vector<float>{1.0f, -2.0f}, {2}, cfg);
  Series p_fused = p_loop.clone();
  Series p_capturable = p_loop.clone();
  const Series g = pluss::make_series(std::vector<float>{0.2f, -0.4f}, {2}, cfg);

  OptionsT loop_opt = base_options;
  loop_opt.execution.lane = UpdateLane::Loop;
  OptionsT fused_opt = base_options;
  fused_opt.execution.lane = UpdateLane::Fused;
  fused_opt.execution.strict = true;
  OptionsT cap_opt = fused_opt;
  cap_opt.execution.capturable = true;

  OptimizerT loop(std::vector<Series*>{&p_loop}, loop_opt);
  OptimizerT fused(std::vector<Series*>{&p_fused}, fused_opt);
  OptimizerT capturable(std::vector<Series*>{&p_capturable}, cap_opt);
  for (int i = 0; i < steps; ++i) {
    loop.step(std::vector<Series>{g});
    fused.step(std::vector<Series>{g});
    capturable.step(std::vector<Series>{g});
  }

  if (p_fused.platform() != pluss::pMPS || p_capturable.platform() != pluss::pMPS ||
      !approx_vec(vec_f64(p_loop), vec_f64(p_fused), 1e-5) ||
      !approx_vec(vec_f64(p_loop), vec_f64(p_capturable), 1e-5)) {
    if (err) *err << name << ": MPS fused/capturable parity mismatch\n";
    return false;
  }
  const auto& fused_lane = fused.last_step_lane_telemetry();
  const auto& cap_lane = capturable.last_step_lane_telemetry();
  if (fused_lane.empty() || cap_lane.empty() ||
      fused_lane.front().resolved != UpdateLane::Fused ||
      cap_lane.front().resolved != UpdateLane::Fused ||
      !fused_lane.front().fused_supported ||
      !cap_lane.front().capturable_supported) {
    if (err) *err << name << ": MPS optimizer lane telemetry mismatch\n";
    return false;
  }
  return true;
}

bool run_mps_lane_checks(std::ostream* err = nullptr) {
  if (!pluss::mps::is_ready()) return true;
  ScopedEnvVar no_fallback("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  {
    pluss::nn::optim::SGDOptions opt{};
    opt.learning_rate = 0.05;
    opt.momentum = 0.9;
    opt.nesterov = true;
    if (!run_mps_optimizer_lane_case_<pluss::nn::optim::SGD>("SGD", opt, 8, err)) return false;
  }
  {
    pluss::nn::optim::AdamOptions opt{};
    opt.learning_rate = 1e-3;
    if (!run_mps_optimizer_lane_case_<pluss::nn::optim::Adam>("Adam", opt, 8, err)) return false;
  }
  {
    pluss::nn::optim::RMSpropOptions opt{};
    opt.learning_rate = 1e-3;
    opt.momentum = 0.5;
    if (!run_mps_optimizer_lane_case_<pluss::nn::optim::RMSprop>("RMSprop", opt, 8, err)) return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nn optimizer foreach lane parity and strict behavior") {
  REQUIRE(run_foreach_lane_checks(&std::cerr));
  REQUIRE(run_mps_lane_checks(&std::cerr));
}
#else
int main() {
  if (!run_foreach_lane_checks(&std::cerr)) return 1;
  if (!run_mps_lane_checks(&std::cerr)) return 1;
  std::cout << "nn_optim_foreach_lane_test passed\n";
  return 0;
}
#endif
