#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using Catch::Approx;

} // namespace

TEST_CASE("parallel contiguous copy preserves expanded int64 broadcast lanes", "[iterator][parallel][regression]") {
  const Config i64 = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const std::vector<std::int64_t> shape{1, 2311, 128}; // 295,808 > parallel threshold

  const Series seed = pluss::arange(7, 8, 1, i64);
  const Series expanded =
      seed.reshape({1, 1, 1})
          .expand(std::span<const std::int64_t>(shape.data(), shape.size()), true)
          .contiguous();

  const Series flat = expanded.reshape({expanded.numel()});
  const auto values = flat.to_vec<std::int64_t>();
  REQUIRE(values.size() == static_cast<std::size_t>(expanded.numel()));
  for (std::size_t i = 0; i < values.size(); ++i) {
    REQUIRE(values[i] == 7);
  }
}

TEST_CASE("index_select remains stable on large 3D lane selection", "[index_select][regression]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Config i64 = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);

  const Series x = pluss::rand({40, 2311, 128}, cfg, 20260314ULL);
  const Series idx = pluss::arange(30, 31, 1, i64);
  const Series y = x.index_select(0, idx);

  REQUIRE(y.sizes()[0] == 1);
  REQUIRE(y.sizes()[1] == 2311);
  REQUIRE(y.sizes()[2] == 128);

  const Series ref = x.select(0, 30).unsqueeze(0);
  const Series diff = y.sub(ref).abs().mean();
  REQUIRE(diff.item<double>() == Approx(0.0).margin(1e-7));
}
