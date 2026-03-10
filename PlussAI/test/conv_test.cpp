#include <pluss/pluss.h>
#include <pluss/ai.h>

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

bool run_conv_checks(std::ostream* err = nullptr) {
  const Config cfg = f64();

  const Series conv2_input = pluss::make_series(std::vector<double>{
                                                    1.0, 2.0, 3.0,
                                                    4.0, 5.0, 6.0,
                                                    7.0, 8.0, 9.0},
                                                {1, 1, 3, 3},
                                                cfg);
  const Series conv2_weight = pluss::make_series(std::vector<double>{
                                                     1.0, 0.0,
                                                     0.0, -1.0},
                                                 {1, 1, 2, 2},
                                                 cfg);
  const Series conv2_bias = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);
  const Series conv2_out = pluss::conv(conv2_input, conv2_weight, conv2_bias, 1, 0, 1, 1);
  if (!approx_vec(vec_f64(conv2_out), std::vector<double>{-3.5, -3.5, -3.5, -3.5})) {
    if (err) *err << "conv2d functional output mismatch\n";
    return false;
  }

  pluss::nn::Conv conv2_module(pluss::nn::ConvOptions{
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
  conv2_module->weight() = conv2_weight;
  conv2_module->bias() = conv2_bias;
  if (!approx_vec(vec_f64(conv2_module->forward(conv2_input)), vec_f64(conv2_out))) {
    if (err) *err << "conv2d module output mismatch\n";
    return false;
  }

  const Series conv1_input = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 1, 6}, cfg);
  const Series conv1_weight = pluss::make_series(std::vector<double>{1.0, -1.0, 2.0}, {1, 1, 3}, cfg);
  const Series conv1_bias = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);
  const Series conv1_out = pluss::conv1d(conv1_input, conv1_weight, conv1_bias, 1, 1, 1, 1);
  if (!approx_vec(vec_f64(conv1_out), std::vector<double>{3.5, 5.5, 7.5, 9.5, 11.5, -0.5})) {
    if (err) *err << "conv1d functional output mismatch\n";
    return false;
  }

  const Series conv1_unbatched = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 6}, cfg);
  if (!approx_vec(vec_f64(pluss::conv1d(conv1_unbatched, conv1_weight, conv1_bias, 1, 1, 1, 1)),
                  std::vector<double>{3.5, 5.5, 7.5, 9.5, 11.5, -0.5})) {
    if (err) *err << "conv1d unbatched output mismatch\n";
    return false;
  }

  pluss::nn::Conv1d conv1_module(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 1,
      .dilation = 1,
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 21ULL,
  });
  conv1_module->weight() = conv1_weight;
  conv1_module->bias() = conv1_bias;
  if (!approx_vec(vec_f64(conv1_module->forward(conv1_input)), vec_f64(conv1_out))) {
    if (err) *err << "conv1d module output mismatch\n";
    return false;
  }

  const Series causal_input = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {1, 1, 4}, cfg);
  const Series causal_weight = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0}, {1, 1, 3}, cfg);
  const auto causal_pad = pluss::causal_padding1d(3, 1);
  const Series causal_out = pluss::conv1d(causal_input, causal_weight, Series(), causal_pad, 1, 1, 1);
  if (!approx_vec(vec_f64(causal_out), std::vector<double>{1.0, 2.0, 3.0, 4.0})) {
    if (err) *err << "conv1d causal output mismatch\n";
    return false;
  }

  pluss::nn::Conv1d causal_module(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 0,
      .padding_lr = causal_pad,
      .dilation = 1,
      .groups = 1,
      .bias = false,
      .cfg = cfg,
      .seed = 25ULL,
  });
  causal_module->weight() = causal_weight;
  if (!approx_vec(vec_f64(causal_module->forward(causal_input)), vec_f64(causal_out))) {
    if (err) *err << "conv1d causal module mismatch\n";
    return false;
  }

  const Series tcn_signal = pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0}, {1, 1, 8}, cfg);
  const Series tcn_k1 = pluss::make_series(std::vector<double>{0.25, 0.5, 0.25}, {1, 1, 3}, cfg);
  const Series tcn_k2 = pluss::make_series(std::vector<double>{1.0, 0.0, -1.0}, {1, 1, 3}, cfg);
  const Series stage1 = pluss::conv1d(tcn_signal, tcn_k1, Series(), pluss::causal_padding1d(3, 1), 1, 1, 1).relu();
  const Series stage2_func = pluss::conv1d(stage1, tcn_k2, Series(), pluss::causal_padding1d(3, 2), 1, 2, 1);

  pluss::nn::Conv1d tcn_layer1(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 0,
      .padding_lr = pluss::causal_padding1d(3, 1),
      .dilation = 1,
      .groups = 1,
      .bias = false,
      .cfg = cfg,
      .seed = 31ULL,
  });
  pluss::nn::Conv1d tcn_layer2(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 0,
      .padding_lr = pluss::causal_padding1d(3, 2),
      .dilation = 2,
      .groups = 1,
      .bias = false,
      .cfg = cfg,
      .seed = 37ULL,
  });
  tcn_layer1->weight() = tcn_k1;
  tcn_layer2->weight() = tcn_k2;
  const Series stage2_module = tcn_layer2->forward(tcn_layer1->forward(tcn_signal).relu());
  if (!approx_vec(vec_f64(stage2_module), vec_f64(stage2_func))) {
    if (err) *err << "strict causal TCN-style module stack mismatch\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("conv functional and module surfaces stay aligned") {
  REQUIRE(run_conv_checks());
}
#else
int main() {
  if (!run_conv_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "conv_test passed\n";
  return 0;
}
#endif
