#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("float16 and bfloat16 factories and casting", "[fp16][bf16]") {
  auto a = ones({4}, ScalarType::Float16);
  REQUIRE(a.scalar_type() == ScalarType::Float16);
  auto af = a.to(ScalarType::Float32);
  auto* p = reinterpret_cast<const float*>(af.raw_data());
  REQUIRE(p[0] == 1.0f);

  auto b = full({4}, 2.5, ScalarType::BFloat16);
  REQUIRE(b.scalar_type() == ScalarType::BFloat16);
  auto bf = b.to(ScalarType::Float32);
  auto* q = reinterpret_cast<const float*>(bf.raw_data());
  // bfloat16 has limited precision; 2.5 should be exact
  REQUIRE(q[1] == 2.5f);

  // mixed f16 + bf16 promotion should go to float32
  auto c = maximum(a.to(ScalarType::Float16), b.to(ScalarType::BFloat16));
  REQUIRE(c.scalar_type() == ScalarType::Float32);
}

TEST_CASE("comparisons and clamp work on float16/bfloat16", "[fp16][bf16]") {
  auto a = full({3}, 1.0, ScalarType::Float16);
  auto b = full({3}, 2.0, ScalarType::Float16);
  auto m = lt(a, b);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
  auto* mp = reinterpret_cast<const std::uint8_t*>(m.raw_data());
  REQUIRE(mp[0] == 1);

  auto x = full({3}, 5.0, ScalarType::BFloat16);
  auto y = clamp(x, 1.0, 3.0);
  REQUIRE(y.scalar_type() == ScalarType::BFloat16);
  auto yf = y.to(ScalarType::Float32);
  auto* yp = reinterpret_cast<const float*>(yf.raw_data());
  REQUIRE(yp[2] == 3.0f);
}
