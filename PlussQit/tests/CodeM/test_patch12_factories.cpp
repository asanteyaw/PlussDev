#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;
using pluss::series::arange;
using pluss::series::eye;
using pluss::series::linspace;
using pluss::series::poisson;
using pluss::series::rand;
using pluss::series::randn;



TEST_CASE("arange/linspace/eye factories", "[factory]") {
  auto a = arange(0.0, 5.0, 1.0, ScalarType::Int32);
  REQUIRE(a.numel() == 5);
  REQUIRE(a.scalar_type() == ScalarType::Int32);

  auto l = linspace(0.0, 1.0, 5, ScalarType::Float32);
  REQUIRE(l.numel() == 5);

  auto e = eye(3, -1, ScalarType::Float16);
  REQUIRE(e.sizes()[0] == 3);
  REQUIRE(e.sizes()[1] == 3);
}

TEST_CASE("rand/randn deterministic with seed", "[factory]") {
  auto a = rand({4}, ScalarType::Float32, 123);
  auto b = rand({4}, ScalarType::Float32, 123);
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  auto* bp = reinterpret_cast<const float*>(b.raw_data());
  REQUIRE(ap[0] == bp[0]);

  auto n0 = randn({4}, ScalarType::Float64, 999);
  auto n1 = randn({4}, ScalarType::Float64, 999);
  auto* n0p = reinterpret_cast<const double*>(n0.raw_data());
  auto* n1p = reinterpret_cast<const double*>(n1.raw_data());
  REQUIRE(n0p[2] == n1p[2]);
}

TEST_CASE("rng_seed drives deterministic zero-seed factory stream", "[factory][rng_seed]") {
  pluss::tils::rng_seed(424242u);
  const auto seed0 = pluss::tils::rng_seed();
  REQUIRE(seed0 == 424242u);

  const auto a0 = rand({8}, ScalarType::Float32);
  const auto a1 = rand({8}, ScalarType::Float32);
  REQUIRE(a0.to_vec<float>() != a1.to_vec<float>());

  pluss::tils::rng_seed(424242u);
  const auto b0 = rand({8}, ScalarType::Float32);
  const auto b1 = rand({8}, ScalarType::Float32);
  REQUIRE(b0.to_vec<float>() == a0.to_vec<float>());
  REQUIRE(b1.to_vec<float>() == a1.to_vec<float>());

  pluss::tils::rng_seed(999u);
  const auto c0 = randn({8}, ScalarType::Float32);
  pluss::tils::rng_seed(999u);
  const auto c1 = randn({8}, ScalarType::Float32);
  REQUIRE(c0.to_vec<float>() == c1.to_vec<float>());

  const auto explicit_seed_a = rand({8}, ScalarType::Float32, 7u);
  const auto explicit_seed_b = rand({8}, ScalarType::Float32, 7u);
  REQUIRE(explicit_seed_a.to_vec<float>() == explicit_seed_b.to_vec<float>());
}

TEST_CASE("poisson factory is deterministic and dtype-aware", "[factory][poisson]") {
  const auto a = poisson({2, 4}, 3.25, ScalarType::Int64, 123u);
  const auto b = poisson({2, 4}, 3.25, ScalarType::Int64, 123u);
  REQUIRE(a.sizes()[0] == 2);
  REQUIRE(a.sizes()[1] == 4);
  REQUIRE(a.scalar_type() == ScalarType::Int64);
  REQUIRE(a.to_vec<std::int64_t>() == b.to_vec<std::int64_t>());
  for (const auto v : a.to_vec<std::int64_t>()) {
    REQUIRE(v >= 0);
  }

  const auto f = poisson({3}, 0.0, ScalarType::Float32, 99u);
  REQUIRE(f.scalar_type() == ScalarType::Float32);
  for (const auto v : f.to_vec<float>()) {
    REQUIRE(v == 0.0f);
  }
}
