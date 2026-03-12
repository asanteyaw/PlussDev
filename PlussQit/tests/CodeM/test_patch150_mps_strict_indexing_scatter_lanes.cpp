#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_)) {
      old_ = std::string(old);
    }
#if defined(_WIN32)
    _putenv_s(key_, value);
#else
    setenv(key_, value, 1);
#endif
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
#if defined(_WIN32)
      _putenv_s(key_, old_->c_str());
#else
      setenv(key_, old_->c_str(), 1);
#endif
    } else {
#if defined(_WIN32)
      _putenv_s(key_, "");
#else
      unsetenv(key_);
#endif
    }
  }

private:
  const char* key_;
  std::optional<std::string> old_;
};

void require_close_(
    const std::vector<float>& got,
    const std::vector<float>& want,
    const char* label,
    float tol = 2e-2f) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    INFO(label << " i=" << i << " got=" << got[i] << " want=" << want[i] << " tol=" << tol);
    REQUIRE(std::fabs(got[i] - want[i]) <= tol);
  }
}

template <class T>
void require_equal_(const std::vector<T>& got, const std::vector<T>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("MPS strict gather/scatter supports high-rank safe dtype lanes", "[mps][parity][torch][strict][indexing][scatter]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  // Gather lane: float16 input + int64 index on rank-3 tensor.
  const auto gather_in_cpu = pluss::make_series(
                                 std::vector<float>{
                                     0.1f, 0.2f, 0.3f, 0.4f,
                                     1.1f, 1.2f, 1.3f, 1.4f,
                                     2.1f, 2.2f, 2.3f, 2.4f,
                                     3.1f, 3.2f, 3.3f, 3.4f,
                                     4.1f, 4.2f, 4.3f, 4.4f,
                                     5.1f, 5.2f, 5.3f, 5.4f},
                                 {2, 3, 4})
                                 .to(pluss::ScalarType::Float16);
  const auto gather_idx_cpu = pluss::make_series(
      std::vector<std::int64_t>{
          3, 1, 0, 2, 2, 0, 1, 3,
          1, 3, 2, 0, 0, 2, 3, 1,
          3, 1, 2, 0, 1, 0, 3, 2},
      {2, 3, 4});
  const auto gather_out_cpu = gather_in_cpu.gather(2, gather_idx_cpu);
  const auto gather_out_mps = gather_in_cpu.to(pluss::pMPS).gather(2, gather_idx_cpu.to(pluss::pMPS));
  REQUIRE(gather_out_mps.platform() == pluss::pMPS);
  require_close_(
      gather_out_mps.to(pluss::pCPU).to(pluss::ScalarType::Float32).to_vec<float>(),
      gather_out_cpu.to(pluss::ScalarType::Float32).to_vec<float>(),
      "gather");

  // Scatter lane: int16 exact-roundtrip lane on rank-3 tensor.
  const auto scatter_base_cpu = pluss::Series::zeros({2, 3, 4}, pluss::ScalarType::Int16);
  const auto scatter_idx_cpu = pluss::make_series(
      std::vector<std::int64_t>{
          0, 1, 2, 3,  3, 2, 1, 0,
          1, 1, 2, 2,  0, 0, 3, 3,
          2, 3, 0, 1,  1, 0, 3, 2},
      {2, 3, 4});
  const auto scatter_src_cpu = pluss::make_series(
      std::vector<std::int16_t>{
          5, 4, 3, 2,  1, 0, -1, -2,
          7, 6, 5, 4,  3, 2, 1, 0,
          9, 8, 7, 6,  5, 4, 3, 2},
      {2, 3, 4});
  const auto scatter_out_cpu = scatter_base_cpu.scatter(2, scatter_idx_cpu, scatter_src_cpu);
  const auto scatter_out_mps =
      scatter_base_cpu.to(pluss::pMPS).scatter(2, scatter_idx_cpu.to(pluss::pMPS), scatter_src_cpu.to(pluss::pMPS));
  REQUIRE(scatter_out_mps.platform() == pluss::pMPS);
  require_equal_(
      scatter_out_mps.to(pluss::pCPU).to_vec<std::int16_t>(),
      scatter_out_cpu.to_vec<std::int16_t>());

}

TEST_CASE("MPS strict gather/scatter rejects unsupported non-exact lane", "[mps][parity][torch][strict][indexing][scatter][policy]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable on this host");
    return;
  }
  const ScopedEnvVar fallback_off("PLUSS_MPS_ALLOW_CPU_FALLBACK", "0");
  const ScopedEnvVar native_take_put("PLUSS_MPS_NATIVE_TAKE_PUT", "1");

  const auto idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3}, {4}).to(pluss::pMPS);

  // int64 data is intentionally outside the exact f32 roundtrip lane.
  const auto x_i64 = pluss::Series::ones({4}, pluss::ScalarType::Int64).to(pluss::pMPS);
  try {
    (void)x_i64.gather(0, idx);
    FAIL("strict gather should fail for unsupported non-exact dtype lane");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("no native MPS kernel yet") != std::string::npos);
  }

  const auto base_i64 = pluss::Series::zeros({4}, pluss::ScalarType::Int64).to(pluss::pMPS);
  const auto src_i64 = pluss::Series::ones({4}, pluss::ScalarType::Int64).to(pluss::pMPS);
  try {
    (void)base_i64.scatter(0, idx, src_i64);
    FAIL("strict scatter should fail for unsupported non-exact dtype lane");
  } catch (const std::exception& e) {
    REQUIRE(std::string(e.what()).find("no native MPS kernel yet") != std::string::npos);
  }
}
