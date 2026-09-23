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
  const Series conv2_out = pluss::nn::conv(conv2_input, conv2_weight, conv2_bias, 1, 0, 1, 1);
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

  const Series conv3_input = pluss::make_series(std::vector<double>{
                                                    1.0, 2.0, 3.0,
                                                    4.0, 5.0, 6.0,
                                                    7.0, 8.0, 9.0,
                                                    10.0, 11.0, 12.0,
                                                    13.0, 14.0, 15.0,
                                                    16.0, 17.0, 18.0,
                                                    19.0, 20.0, 21.0,
                                                    22.0, 23.0, 24.0,
                                                    25.0, 26.0, 27.0},
                                                {1, 1, 3, 3, 3},
                                                cfg);
  const Series conv3_weight = pluss::make_series(std::vector<double>{
                                                     1.0, 0.0,
                                                     0.0, 0.0,
                                                     0.0, 0.0,
                                                     0.0, -1.0},
                                                 {1, 1, 2, 2, 2},
                                                 cfg);
  const Series conv3_bias = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);
  const Series conv3_out = pluss::nn::conv3d(conv3_input, conv3_weight, conv3_bias, 1, 0, 1, 1);
  if (!approx_vec(vec_f64(conv3_out), std::vector<double>{-12.5, -12.5, -12.5, -12.5, -12.5, -12.5, -12.5, -12.5})) {
    if (err) *err << "conv3d functional output mismatch\n";
    return false;
  }

  pluss::nn::Conv3d conv3_module(pluss::nn::Conv3dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = {2, 2, 2},
      .stride = {1, 1, 1},
      .padding = {0, 0, 0},
      .dilation = {1, 1, 1},
      .groups = 1,
      .bias = true,
      .cfg = cfg,
      .seed = 17ULL,
  });
  conv3_module->weight() = conv3_weight;
  conv3_module->bias() = conv3_bias;
  if (!approx_vec(vec_f64(conv3_module->forward(conv3_input)), vec_f64(conv3_out))) {
    if (err) *err << "conv3d module output mismatch\n";
    return false;
  }

  const Series conv1_input = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 1, 6}, cfg);
  const Series conv1_weight = pluss::make_series(std::vector<double>{1.0, -1.0, 2.0}, {1, 1, 3}, cfg);
  const Series conv1_bias = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);
  const Series conv1_out = pluss::nn::conv1d(conv1_input, conv1_weight, conv1_bias, 1, 1, 1, 1);
  if (!approx_vec(vec_f64(conv1_out), std::vector<double>{3.5, 5.5, 7.5, 9.5, 11.5, -0.5})) {
    if (err) *err << "conv1d functional output mismatch\n";
    return false;
  }

  // Multi-channel regression guard: every other case in this file uses
  // in_channels==1, which can't catch a channel/kernel axis mixup in the
  // unfold-based patch extraction (conv2d_impl_ in convNd.h) -- a real bug
  // found and fixed this way (the second unfold call was targeting the
  // just-created kernel-height window dim instead of the width dim, an
  // error invisible whenever in_channels==1 since there's no channel axis
  // to get crossed with the kernel axes). Expected values cross-checked
  // against an independent reference cross-correlation implementation.
  {
    const Series mc_input = pluss::make_series(
        std::vector<double>{
            1.0, 1.1, 1.2, 1.3, 1.4, 1.5,  1.6, 1.7, 1.8, 1.9, 2.0, 2.1,
            2.2, 2.3, 2.4, 2.5, 2.6, 2.7,  2.8, 2.9, 3.0, 3.1, 3.2, 3.3},
        {2, 2, 6}, cfg);
    const Series mc_weight = pluss::make_series(
        std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8}, {2, 2, 2}, cfg);
    const Series mc_out = pluss::nn::conv1d(mc_input, mc_weight, Series(), 1, 0, 1, 1);
    const std::vector<double> mc_expected{
        1.48, 1.58, 1.68, 1.78, 1.88,
        3.64, 3.90, 4.16, 4.42, 4.68,
        2.68, 2.78, 2.88, 2.98, 3.08,
        6.76, 7.02, 7.28, 7.54, 7.80};
    if (!approx_vec(vec_f64(mc_out), mc_expected)) {
      if (err) *err << "conv1d multi-channel output mismatch\n";
      return false;
    }
  }

  const Series conv1_unbatched = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1, 6}, cfg);
  if (!approx_vec(vec_f64(pluss::nn::conv1d(conv1_unbatched, conv1_weight, conv1_bias, 1, 1, 1, 1)),
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
  const auto causal_pad = pluss::nn::causal_padding1d(3, 1);
  const Series causal_out = pluss::nn::conv1d(causal_input, causal_weight, Series(), causal_pad, 1, 1, 1);
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
  const Series stage1 = pluss::nn::conv1d(tcn_signal, tcn_k1, Series(), pluss::nn::causal_padding1d(3, 1), 1, 1, 1).relu();
  const Series stage2_func = pluss::nn::conv1d(stage1, tcn_k2, Series(), pluss::nn::causal_padding1d(3, 2), 1, 2, 1);

  pluss::nn::Conv1d tcn_layer1(pluss::nn::Conv1dOptions{
      .in_channels = 1,
      .out_channels = 1,
      .kernel_size = 3,
      .stride = 1,
      .padding = 0,
      .padding_lr = pluss::nn::causal_padding1d(3, 1),
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
      .padding_lr = pluss::nn::causal_padding1d(3, 2),
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
