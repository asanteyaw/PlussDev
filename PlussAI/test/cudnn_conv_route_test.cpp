#include <pluss/pluss.h>
#include <pluss/ai.h>

#include "NodeK/kernels/cudnn_conv.h"

#include <iostream>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

bool has_shape(const pluss::Series& x, std::initializer_list<std::int64_t> expected) {
  if (x.dim() != static_cast<std::int64_t>(expected.size())) return false;
  std::size_t i = 0;
  for (const auto d : expected) {
    if (x.sizes()[i++] != d) return false;
  }
  return true;
}

bool run_cudnn_conv_route_checks(std::ostream* err = nullptr) {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::rand({2, 4, 8, 8}, cfg, 11ULL);
  const pluss::Series w = pluss::rand({6, 2, 3, 3}, cfg, 13ULL);
  const pluss::Series b = pluss::rand({6}, cfg, 17ULL);

  std::string why{};
  const bool supported = pluss::ai::kernels::cudnn::supports_conv2d(
      x,
      w,
      b,
      pluss::ai::kernels::cudnn::Conv2dGeometry{
          .stride = {1, 1},
          .padding = {1, 1},
          .dilation = {1, 1},
          .groups = 2,
      },
      false,
      &why);
  if (supported) {
    if (err) *err << "CPU tensors should not report cuDNN support\n";
    return false;
  }
  if (why.empty()) {
    if (err) *err << "supports_conv2d should provide a reason for unsupported route\n";
    return false;
  }

  const auto routed = pluss::ai::kernels::cudnn::try_conv2d(
      x,
      w,
      b,
      pluss::ai::kernels::cudnn::Conv2dGeometry{
          .stride = {1, 1},
          .padding = {1, 1},
          .dilation = {1, 1},
          .groups = 2,
      },
      false);
  if (routed.has_value()) {
    if (err) *err << "CPU tensors should not take cuDNN route\n";
    return false;
  }

  const pluss::Series y = pluss::conv(x, w, b, std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{1, 1}, std::array<std::int64_t, 2>{1, 1}, 2);
  if (!has_shape(y, {2, 6, 8, 8})) {
    if (err) *err << "conv output shape mismatch\n";
    return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN conv route preserves CPU fallback behavior") {
  REQUIRE(run_cudnn_conv_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_cudnn_conv_route_checks(&std::cerr)) return 1;
  std::cout << "cudnn_conv_route_test: ok\n";
  return 0;
}
#endif
