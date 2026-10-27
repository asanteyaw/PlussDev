// Documents a real, by-design behavior discovered while investigating MPS
// GRU training: MPS's RNG does NOT reproduce CPU's RNG for the same seed.
// This matches real torch's own per-backend RNG streams -- torch.manual_seed
// never promised bit-identical output between CPU/CUDA/MPS either, since
// each backend uses its own generator. Pinned here so nobody re-discovers
// this by chasing a phantom "wrong results" bug the way this test's own
// investigation initially did (see nn_mps_gru_native_route_test.cpp).
#include <pluss/ai.h>

#include <cmath>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

namespace {

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

}  // namespace

TEST_CASE("MPS randn does not reproduce CPU randn for the same seed", "[mps][rng][stochastic]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const Config cpu_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Config mps_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);

  const auto cpu_v = vec_f64(pluss::series::randn({8}, cpu_cfg, /*seed=*/42));
  const auto mps_v = vec_f64(pluss::series::randn({8}, mps_cfg, /*seed=*/42));

  REQUIRE(cpu_v.size() == mps_v.size());
  bool any_differs = false;
  for (std::size_t i = 0; i < cpu_v.size(); ++i) {
    if (std::abs(cpu_v[i] - mps_v[i]) > 1e-4) any_differs = true;
  }
  // This REQUIRE documents the CURRENT, by-design state (different RNG
  // streams per platform), not a bug -- if this ever starts failing (i.e.
  // the two streams start matching), that's a real behavior change worth
  // noticing, not a regression to "fix".
  REQUIRE(any_differs);
}
