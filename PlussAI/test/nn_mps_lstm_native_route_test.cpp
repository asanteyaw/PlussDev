#include <pluss/ai.h>

#include "NodeK/kernels/mps_rnn.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

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

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

bool close_vec(const std::vector<double>& a, const std::vector<double>& b, double atol, double rtol) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double diff = std::abs(a[i] - b[i]);
    if (diff > atol + rtol * std::abs(b[i])) return false;
  }
  return true;
}

pluss::ai::rnn::NetWeight make_weight_block(const Config& cfg) {
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(
          std::vector<float>{
              0.12f, -0.04f,
              0.03f, 0.09f,
              -0.08f, 0.05f,
              0.07f, -0.02f,
              0.11f, 0.01f,
              -0.03f, 0.06f,
              0.04f, -0.07f,
              0.02f, 0.08f,
              -0.05f, 0.10f,
              0.09f, -0.01f,
              0.06f, 0.03f,
              -0.02f, 0.04f,
          },
          {12, 2},
          cfg),
      .weight_hh = pluss::make_series(
          std::vector<float>{
              0.05f, -0.01f, 0.03f,
              0.02f, 0.04f, -0.02f,
              -0.03f, 0.06f, 0.01f,
              0.07f, -0.05f, 0.02f,
              0.01f, 0.03f, 0.04f,
              -0.04f, 0.02f, 0.05f,
              0.06f, -0.02f, 0.01f,
              0.03f, 0.05f, -0.01f,
              -0.02f, 0.04f, 0.06f,
              0.05f, 0.01f, -0.03f,
              0.02f, -0.04f, 0.03f,
              0.04f, 0.06f, -0.02f,
          },
          {12, 3},
          cfg),
      .bias = pluss::make_series(
          std::vector<float>{0.01f, -0.02f, 0.03f, 0.04f, -0.01f, 0.02f, -0.03f, 0.01f, 0.02f, -0.04f, 0.03f, -0.02f},
          {12},
          cfg),
  };
}

pluss::ai::rnn::NetWeight make_weight_block_input3(const Config& cfg) {
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(
          std::vector<float>{
              0.05f, -0.01f, 0.03f,
              0.02f, 0.04f, -0.02f,
              -0.03f, 0.06f, 0.01f,
              0.07f, -0.05f, 0.02f,
              0.01f, 0.03f, 0.04f,
              -0.04f, 0.02f, 0.05f,
              0.06f, -0.02f, 0.01f,
              0.03f, 0.05f, -0.01f,
              -0.02f, 0.04f, 0.06f,
              0.05f, 0.01f, -0.03f,
              0.02f, -0.04f, 0.03f,
              0.04f, 0.06f, -0.02f,
          },
          {12, 3},
          cfg),
      .weight_hh = pluss::make_series(
          std::vector<float>{
              -0.02f, 0.03f, 0.05f,
              0.04f, -0.01f, 0.02f,
              0.01f, 0.05f, -0.04f,
              -0.03f, 0.02f, 0.06f,
              0.02f, -0.05f, 0.01f,
              0.03f, 0.04f, -0.02f,
              -0.01f, 0.06f, 0.03f,
              0.05f, -0.03f, 0.02f,
              0.04f, 0.01f, -0.05f,
              -0.02f, 0.05f, 0.04f,
              0.06f, -0.01f, 0.03f,
              0.01f, 0.02f, -0.04f,
          },
          {12, 3},
          cfg),
      .bias = pluss::make_series(
          std::vector<float>{-0.01f, 0.02f, 0.01f, -0.03f, 0.04f, -0.02f, 0.03f, 0.02f, -0.04f, 0.01f, -0.02f, 0.03f},
          {12},
          cfg),
  };
}

pluss::ai::rnn::NetWeight make_weight_block_for_input(const Config& cfg, std::int64_t input_size, float shift) {
  std::vector<float> weight_ih(static_cast<std::size_t>(12 * input_size));
  for (std::size_t i = 0; i < weight_ih.size(); ++i) {
    const int bucket = static_cast<int>((i * 7 + static_cast<std::size_t>(shift * 100.0f)) % 11);
    weight_ih[i] = (static_cast<float>(bucket) - 5.0f) * 0.011f;
  }
  std::vector<float> weight_hh(36);
  for (std::size_t i = 0; i < weight_hh.size(); ++i) {
    const int bucket = static_cast<int>((i * 5 + static_cast<std::size_t>(shift * 80.0f)) % 13);
    weight_hh[i] = (static_cast<float>(bucket) - 6.0f) * 0.009f;
  }
  std::vector<float> bias(12);
  for (std::size_t i = 0; i < bias.size(); ++i) {
    const int bucket = static_cast<int>((i * 3 + static_cast<std::size_t>(shift * 60.0f)) % 7);
    bias[i] = (static_cast<float>(bucket) - 3.0f) * 0.01f;
  }
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(weight_ih, {12, input_size}, cfg),
      .weight_hh = pluss::make_series(weight_hh, {12, 3}, cfg),
      .bias = pluss::make_series(bias, {12}, cfg),
  };
}

template <class MakeParams>
bool run_case(const Series& x,
              const Series& hx,
              const Series& cx,
              pluss::ai::rnn::NetConfig cfg,
              MakeParams make_params,
              const char* label,
              std::ostream* err) {
  std::vector<pluss::ai::rnn::NetWeight> params = make_params();
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  cfg.num_layers = static_cast<std::int64_t>(params.size()) / dirs;

  Series ref_y;
  Series ref_h;
  Series ref_c;
  {
    ScopedEnvVar route_off("PLUSS_MPS_LSTM_ROUTE", "0");
    auto [y, state] = pluss::ai::rnn::lstm(
        x,
        pluss::ai::rnn::LSTMState{hx, cx},
        pluss::msharp::RangeView::Of<pluss::ai::rnn::NetWeight>(params),
        cfg);
    ref_y = y;
    ref_h = std::get<0>(state);
    ref_c = std::get<1>(state);
  }

  Series got_y;
  Series got_h;
  Series got_c;
  {
    ScopedEnvVar route_on("PLUSS_MPS_LSTM_ROUTE", "1");
    ScopedEnvVar strict("PLUSS_MPS_LSTM_ROUTE_STRICT", "1");
    auto [y, state] = pluss::ai::rnn::lstm(
        x,
        pluss::ai::rnn::LSTMState{hx, cx},
        pluss::msharp::RangeView::Of<pluss::ai::rnn::NetWeight>(params),
        cfg);
    got_y = y;
    got_h = std::get<0>(state);
    got_c = std::get<1>(state);
  }

  const bool ok_y = close_vec(vec_f64(got_y), vec_f64(ref_y), 2e-4, 2e-4);
  const bool ok_h = close_vec(vec_f64(got_h), vec_f64(ref_h), 2e-4, 2e-4);
  const bool ok_c = close_vec(vec_f64(got_c), vec_f64(ref_c), 2e-4, 2e-4);
  if (!ok_y || !ok_h || !ok_c) {
    if (err) *err << label << " native MPS LSTM route mismatch against Series fallback\n";
    return false;
  }

  auto gradient_trace = [&](const char* route_enabled) {
    ScopedEnvVar route("PLUSS_MPS_LSTM_ROUTE", route_enabled);
    ScopedEnvVar strict("PLUSS_MPS_LSTM_ROUTE_STRICT", route_enabled);
    Series xg = x.clone();
    Series hxg = hx.clone();
    Series cxg = cx.clone();
    xg.tracks_slope_(true);
    hxg.tracks_slope_(true);
    cxg.tracks_slope_(true);
    std::vector<pluss::ai::rnn::NetWeight> pg = make_params();
    for (auto& p : pg) {
      p.weight_ih.tracks_slope_(true);
      p.weight_hh.tracks_slope_(true);
      p.bias.tracks_slope_(true);
    }
    auto [y, state] = pluss::ai::rnn::lstm(
        xg,
        pluss::ai::rnn::LSTMState{hxg, cxg},
        pluss::msharp::RangeView::Of<pluss::ai::rnn::NetWeight>(pg),
        cfg);
    const Series h = std::get<0>(state);
    const Series c = std::get<1>(state);
    const Series loss = y.square().sum().add(h.square().sum()).add(c.square().sum());
    loss.reverse();
    std::vector<std::vector<double>> grads;
    grads.push_back(vec_f64(xg.sensitivity()));
    grads.push_back(vec_f64(hxg.sensitivity()));
    grads.push_back(vec_f64(cxg.sensitivity()));
    for (const auto& p : pg) {
      grads.push_back(vec_f64(p.weight_ih.sensitivity()));
      grads.push_back(vec_f64(p.weight_hh.sensitivity()));
      grads.push_back(vec_f64(p.bias.sensitivity()));
    }
    return grads;
  };

  const auto ref_grads = gradient_trace("0");
  const auto got_grads = gradient_trace("1");
  if (ref_grads.size() != got_grads.size()) {
    if (err) *err << label << " native MPS LSTM gradient trace arity mismatch\n";
    return false;
  }
  for (std::size_t i = 0; i < ref_grads.size(); ++i) {
    if (!close_vec(got_grads[i], ref_grads[i], 2e-3, 2e-3)) {
      if (err) *err << label << " native MPS LSTM gradient mismatch at slot " << i << "\n";
      return false;
    }
  }
  return true;
}

bool run_native_lstm_route_check(std::ostream* err = nullptr) {
  if (!pluss::mps::is_ready()) {
    return true;
  }
  if (!pluss::ai::kernels::mps::lstm_route_ready()) {
    return true;
  }

  const Config mps = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
  pluss::ai::rnn::NetConfig cfg{};
  cfg.training = false;
  cfg.batch_first = false;
  cfg.bidirectional = false;
  cfg.dropout = 0.0;
  cfg.num_layers = 1;
  cfg.use_bias = true;
  cfg.split_bias = false;
  cfg.pre_compute_input = false;
  cfg.proj_size = 0;
  cfg.cfg = mps;

  const Series x = pluss::make_series(
      std::vector<float>{
          -0.2f, 0.1f,
          0.0f, 0.3f,
          0.4f, -0.1f,
          0.2f, 0.5f,
          -0.3f, 0.2f,
          0.1f, -0.4f,
      },
      {3, 2, 2},
      mps).contiguous();
  const Series hx1 = pluss::make_series(std::vector<float>{0.01f, -0.02f, 0.03f, 0.04f, -0.01f, 0.02f}, {1, 2, 3}, mps)
                         .contiguous();
  const Series cx1 = pluss::make_series(std::vector<float>{0.03f, 0.01f, -0.02f, -0.01f, 0.02f, 0.04f}, {1, 2, 3}, mps)
                         .contiguous();
  if (!run_case(x, hx1, cx1, cfg, [&]() { return std::vector<pluss::ai::rnn::NetWeight>{make_weight_block(mps)}; }, "single-layer", err)) {
    return false;
  }

  const Series hx2 = pluss::make_series(
      std::vector<float>{
          0.01f, -0.02f, 0.03f,
          0.04f, -0.01f, 0.02f,
          -0.03f, 0.02f, 0.01f,
          0.05f, -0.04f, 0.03f,
      },
      {2, 2, 3},
      mps).contiguous();
  const Series cx2 = pluss::make_series(
      std::vector<float>{
          0.03f, 0.01f, -0.02f,
          -0.01f, 0.02f, 0.04f,
          0.02f, -0.03f, 0.01f,
          -0.04f, 0.03f, 0.05f,
      },
      {2, 2, 3},
      mps).contiguous();
  if (!run_case(
      x,
      hx2,
      cx2,
      cfg,
      [&]() {
        return std::vector<pluss::ai::rnn::NetWeight>{make_weight_block(mps), make_weight_block_input3(mps)};
      },
      "stacked",
      err)) {
    return false;
  }

  pluss::ai::rnn::NetConfig bidir_cfg = cfg;
  bidir_cfg.training = true;
  bidir_cfg.bidirectional = true;
  bidir_cfg.dropout = 0.25;
  bidir_cfg.seed = 37;
  const Series hx_bidir = pluss::make_series(
      std::vector<float>{
          0.01f, -0.02f, 0.03f,
          0.04f, -0.01f, 0.02f,
          -0.03f, 0.02f, 0.01f,
          0.05f, -0.04f, 0.03f,
          0.02f, 0.01f, -0.03f,
          -0.02f, 0.04f, 0.05f,
          0.03f, -0.05f, 0.02f,
          0.01f, 0.03f, -0.04f,
      },
      {4, 2, 3},
      mps).contiguous();
  const Series cx_bidir = pluss::make_series(
      std::vector<float>{
          0.03f, 0.01f, -0.02f,
          -0.01f, 0.02f, 0.04f,
          0.02f, -0.03f, 0.01f,
          -0.04f, 0.03f, 0.05f,
          -0.02f, 0.05f, 0.01f,
          0.04f, -0.01f, 0.03f,
          0.01f, -0.04f, 0.02f,
          0.05f, 0.02f, -0.03f,
      },
      {4, 2, 3},
      mps).contiguous();
  return run_case(
      x,
      hx_bidir,
      cx_bidir,
      bidir_cfg,
      [&]() {
        return std::vector<pluss::ai::rnn::NetWeight>{
            make_weight_block_for_input(mps, 2, 0.1f),
            make_weight_block_for_input(mps, 2, 0.2f),
            make_weight_block_for_input(mps, 6, 0.3f),
            make_weight_block_for_input(mps, 6, 0.4f),
        };
      },
      "bidirectional-dropout",
      err);
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("native MPS LSTM inference route matches Series fallback", "[mps][lstm][rnn]") {
  REQUIRE(run_native_lstm_route_check());
}
#else
int main() {
  if (!run_native_lstm_route_check(&std::cerr)) return 1;
  return 0;
}
#endif
