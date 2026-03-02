#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

double scalar_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
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

pluss::ai::rnn::NetWeight make_gru_params() {
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(std::vector<double>{
                                          0.3, -0.1,
                                          0.2, 0.4,
                                          -0.5, 0.6},
                                      {3, 2},
                                      f64()),
      .weight_hh = pluss::make_series(std::vector<double>{
                                          0.7,
                                          -0.2,
                                          0.1},
                                      {3, 1},
                                      f64()),
      .bias_ih = pluss::make_series(std::vector<double>{0.05, -0.03, 0.02}, {3}, f64()),
      .bias_hh = pluss::make_series(std::vector<double>{0.01, 0.02, -0.04}, {3}, f64()),
  };
}

pluss::ai::rnn::NetWeight make_lstm_params() {
  return pluss::ai::rnn::NetWeight{
      .weight_ih = pluss::make_series(std::vector<double>{
                                          0.4, -0.1,
                                          0.2, 0.3,
                                          -0.5, 0.7,
                                          0.1, -0.2},
                                      {4, 2},
                                      f64()),
      .weight_hh = pluss::make_series(std::vector<double>{
                                          0.2,
                                          -0.3,
                                          0.6,
                                          0.5},
                                      {4, 1},
                                      f64()),
      .bias_ih = pluss::make_series(std::vector<double>{0.01, -0.02, 0.03, 0.04}, {4}, f64()),
      .bias_hh = pluss::make_series(std::vector<double>{-0.03, 0.01, 0.02, -0.01}, {4}, f64()),
  };
}

struct TinyForecastImpl final : pluss::nn::Module {
  pluss::nn::Fusion fc1;
  pluss::nn::Softplus softplus;
  pluss::nn::Fusion fc2;
  pluss::Series running_mean;

  explicit TinyForecastImpl(const Config& cfg)
      : Module(true),
        fc1(2, 2, true, cfg, 41ULL),
        softplus(pluss::nn::SoftplusOptions{.beta = 1.0, .threshold = 20.0}),
        fc2(2, 1, true, cfg, 43ULL),
        running_mean(pluss::zeros({2}, cfg)) {
    module_record("fc1", fc1);
    module_record("softplus", softplus);
    module_record("fc2", fc2);
    buffer_record("running_mean", running_mean);
  }

  TinyForecastImpl* operator->() noexcept { return this; }
  const TinyForecastImpl* operator->() const noexcept { return this; }

  Series forward(const Series& input) const {
    return fc2->forward(softplus->forward(fc1->forward(input)));
  }
};

PLUSS_MODULE(TinyForecast);

bool run_rnn_fusion_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();

  const Series s = pluss::make_series(0.23, cfg);
  if (!approx(scalar_f64(s + 1.0), 1.23)) { if (err) *err << "scalar rhs add failed\n"; return false; }
  if (!approx(scalar_f64(1.0 + s), 1.23)) { if (err) *err << "scalar lhs add failed\n"; return false; }
  if (!approx(scalar_f64((s * 2.0) - 0.1), 0.36)) { if (err) *err << "scalar arithmetic chain failed\n"; return false; }

  const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}, cfg);
  const Series w = pluss::make_series(std::vector<double>{1.0, 0.0, 0.0, 1.0, 1.0, -1.0}, {3, 2}, cfg);
  const Series b = pluss::make_series(std::vector<double>{0.1, -0.2, 0.3}, {3}, cfg);
  if (!approx_vec(vec_f64(pluss::fusion(x, w, b)), std::vector<double>{1.1, 1.8, -0.7, 3.1, 3.8, -0.7})) {
    if (err) *err << "fusion output mismatch\n";
    return false;
  }

  TinyForecast model(cfg);
  model->fc1->weight() = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0}, {2, 2}, cfg);
  model->fc1->bias() = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg);
  model->fc2->weight() = pluss::make_series(std::vector<double>{1.0, -1.0}, {1, 2}, cfg);
  model->fc2->bias() = pluss::make_series(std::vector<double>{0.25}, {1}, cfg);
  model->eval();
  if (model->is_training() || model->fc1.is_training() || model->fc2.is_training() || model->softplus.is_training()) {
    if (err) *err << "module_record eval recursion mismatch\n";
    return false;
  }
  model->train();
  if (!model->is_training() || !model->fc1.is_training() || !model->fc2.is_training() || !model->softplus.is_training()) {
    if (err) *err << "module_record train recursion mismatch\n";
    return false;
  }
  model->to(pluss::pCPU, ScalarType::Float64);
  if (model->fc1->weight().platform() != pluss::pCPU ||
      model->fc1->weight().scalar_type() != ScalarType::Float64 ||
      model->running_mean.platform() != pluss::pCPU ||
      model->running_mean.scalar_type() != ScalarType::Float64) {
    if (err) *err << "module to(platform, dtype) mismatch\n";
    return false;
  }
  const auto params = model->parameters();
  if (params.size() != 4) {
    if (err) *err << "module parameters() should collect recursive leaf tensors\n";
    return false;
  }
  const auto named = model->named_parameters();
  if (named.size() != 4 ||
      named[0].name != "fc1.weight" ||
      named[1].name != "fc1.bias" ||
      named[2].name != "fc2.weight" ||
      named[3].name != "fc2.bias") {
    if (err) *err << "module named_parameters() recursion/naming mismatch\n";
    return false;
  }
  if (named[0].series != &model->fc1->weight() || named[3].series != &model->fc2->bias()) {
    if (err) *err << "module named_parameters() should reference live tensors\n";
    return false;
  }
  const auto buffers = model->buffers();
  if (buffers.size() != 1 || buffers[0] != &model->running_mean) {
    if (err) *err << "module buffers() should collect registered buffers\n";
    return false;
  }
  const auto named_buffers = model->named_buffers();
  if (named_buffers.size() != 1 || named_buffers[0].name != "running_mean" || named_buffers[0].series != &model->running_mean) {
    if (err) *err << "module named_buffers() mismatch\n";
    return false;
  }
  const auto modules = model->modules();
  if (modules.size() != 4 || modules.front() != model.get()) {
    if (err) *err << "module modules() should include self and registered descendants\n";
    return false;
  }
  const auto named_modules = model->named_modules();
  if (named_modules.size() != 4 ||
      named_modules[0].name != "" ||
      named_modules[1].name != "fc1" ||
      named_modules[2].name != "softplus" ||
      named_modules[3].name != "fc2") {
    if (err) *err << "module named_modules() recursion/naming mismatch\n";
    return false;
  }
  const Series model_out = model->forward(x);
  if (!approx_vec(vec_f64(model_out), std::vector<double>{0.25, 0.25})) {
    if (err) *err << "activation/module composition mismatch\n";
    return false;
  }

  const pluss::StackArray<Series> stack_items{
      pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg),
      pluss::make_series(std::vector<double>{3.0, 4.0}, {2}, cfg),
  };
  if (!approx_vec(vec_f64(pluss::pile(stack_items, 0)), std::vector<double>{1.0, 2.0, 3.0, 4.0})) {
    if (err) *err << "pile StackArray mismatch\n";
    return false;
  }

  const Series rnn_input = pluss::make_series(std::vector<double>{1.0, 2.0}, {1, 2}, cfg);
  const Series rnn_hidden = pluss::make_series(std::vector<double>{0.5}, {1, 1}, cfg);
  const pluss::ai::rnn::NetWeight rnn_params{
      .weight_ih = pluss::make_series(std::vector<double>{1.0, -1.0}, {1, 2}, cfg),
      .weight_hh = pluss::make_series(std::vector<double>{3.0}, {1, 1}, cfg),
      .bias_ih = pluss::make_series(std::vector<double>{0.25}, {1}, cfg),
      .bias_hh = pluss::make_series(std::vector<double>{-0.5}, {1}, cfg),
  };
  if (!approx(scalar_f64(pluss::ai::rnn::rnn_impl(rnn_input, rnn_hidden, rnn_params, false, "relu")), 0.25)) {
    if (err) *err << "rnn_impl mismatch\n";
    return false;
  }

  const auto gru_params = make_gru_params();
  const Series gru_x = pluss::make_series(std::vector<double>{0.4, -0.2}, {1, 2}, cfg);
  const Series gru_h = pluss::make_series(std::vector<double>{0.3}, {1, 1}, cfg);
  const Series gru_split = pluss::ai::rnn::gru_impl(gru_x, gru_h, gru_params, true);
  const Series gru_unified = pluss::ai::rnn::gru_impl(gru_x, gru_h, gru_params, false);
  if (!(std::abs(scalar_f64(gru_split) - scalar_f64(gru_unified)) > 1e-6)) {
    if (err) *err << "split_bias should change GRU output\n";
    return false;
  }

  pluss::ai::rnn::NetConfig run_cfg;
  run_cfg.num_layers = 1;
  run_cfg.batch_first = true;
  run_cfg.split_bias = true;
  run_cfg.cfg = cfg;
  const Series seq = pluss::make_series(std::vector<double>{
                                            0.4, -0.2,
                                            0.1, 0.3,
                                            -0.5, 0.2,
                                            0.7, -0.1},
                                        {2, 2, 2},
                                        cfg);
  const Series h0 = pluss::zeros({1, 2, 1}, cfg);
  const auto [gru_func_out, gru_func_hn] =
      pluss::gru(seq, h0, std::vector<pluss::ai::rnn::NetWeight>{gru_params}, run_cfg);

  pluss::nn::RNN rnn_module(pluss::nn::RNNOptions{
      .input_size = 2,
      .hidden_size = 1,
      .num_layers = 1,
      .nonlinearity = "relu",
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = true,
      .cfg = cfg,
      .seed = 3ULL,
      .training = true,
  });
  rnn_module->params().front() = rnn_params;
  rnn_module->eval();
  if (rnn_module.is_training() || rnn_module->options().training) {
    if (err) *err << "RNN eval should disable training mode\n";
    return false;
  }
  rnn_module->train();
  if (!rnn_module.is_training() || !rnn_module->options().training) {
    if (err) *err << "RNN train should enable training mode\n";
    return false;
  }
  const auto [rnn_mod_out, rnn_mod_hn] = rnn_module->forward(seq, h0);
  if (rnn_mod_out.sizes()[0] != 2 || rnn_mod_hn.sizes()[0] != 1) {
    if (err) *err << "RNN module shape mismatch\n";
    return false;
  }

  pluss::nn::GRU gru_module(pluss::nn::GRUOptions{
      .input_size = 2,
      .hidden_size = 1,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = true,
      .cfg = cfg,
      .seed = 5ULL,
      .training = true,
  });
  gru_module->params().front() = gru_params;
  const auto [gru_mod_out, gru_mod_hn] = gru_module->forward(seq, h0);
  if (!approx_vec(vec_f64(gru_mod_out), vec_f64(gru_func_out)) ||
      !approx_vec(vec_f64(gru_mod_hn), vec_f64(gru_func_hn))) {
    if (err) *err << "GRU module/functional mismatch\n";
    return false;
  }

  const auto lstm_params = make_lstm_params();
  const Series lstm_h0 = pluss::zeros({1, 2, 1}, cfg);
  const Series lstm_c0 = pluss::zeros({1, 2, 1}, cfg);
  const auto [lstm_func_out, lstm_state] =
      pluss::lstm(seq, lstm_h0, lstm_c0, std::vector<pluss::ai::rnn::NetWeight>{lstm_params},
                  pluss::ai::rnn::NetConfig{
                      .training = true,
                      .batch_first = true,
                      .bidirectional = false,
                      .dropout = 0.0,
                      .num_layers = 1,
                      .use_bias = true,
                      .split_bias = true,
                      .pre_compute_input = true,
                      .proj_size = 0,
                      .nonlinearity = "tanh",
                      .cfg = cfg,
                      .seed = 0ULL,
                  });
  pluss::nn::LSTM lstm_module(pluss::nn::LSTMOptions{
      .input_size = 2,
      .hidden_size = 1,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = true,
      .proj_size = 0,
      .cfg = cfg,
      .seed = 7ULL,
      .training = true,
  });
  lstm_module->params().front() = lstm_params;
  const auto [lstm_mod_out, lstm_mod_state] = lstm_module->forward(seq, {lstm_h0, lstm_c0});
  if (!approx_vec(vec_f64(lstm_mod_out), vec_f64(lstm_func_out)) ||
      !approx_vec(vec_f64(std::get<0>(lstm_mod_state)), vec_f64(std::get<0>(lstm_state)))) {
    if (err) *err << "LSTM module/functional mismatch\n";
    return false;
  }

  const Series conv_input = pluss::make_series(std::vector<double>{
                                                   1.0, 2.0, 3.0,
                                                   4.0, 5.0, 6.0,
                                                   7.0, 8.0, 9.0},
                                               {1, 1, 3, 3},
                                               cfg);
  const Series conv_weight = pluss::make_series(std::vector<double>{
                                                    1.0, 0.0,
                                                    0.0, -1.0},
                                                {1, 1, 2, 2},
                                                cfg);
  const Series conv_bias = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);
  const Series conv_out = pluss::conv(conv_input, conv_weight, conv_bias, 1, 0, 1, 1);
  if (!approx_vec(vec_f64(conv_out), std::vector<double>{-3.5, -3.5, -3.5, -3.5})) {
    if (err) *err << "functional conv mismatch\n";
    return false;
  }

  pluss::nn::Conv conv_module(pluss::nn::ConvOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = {2, 2},
      .stride = {1, 1},
      .padding = {0, 0},
      .dilation = {1, 1},
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 13ULL,
  });
  conv_module->weight() = conv_weight;
  conv_module->bias() = conv_bias;
  if (!approx_vec(vec_f64(conv_module->forward(conv_input)), vec_f64(conv_out))) {
    if (err) *err << "module conv mismatch\n";
    return false;
  }

  return true;
}

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("rnn fusion surface stays aligned") {
  REQUIRE(run_rnn_fusion_checks());
}
#else
int main() {
  if (!run_rnn_fusion_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "rnn_fusion_test passed\n";
  return 0;
}
#endif

} // namespace
