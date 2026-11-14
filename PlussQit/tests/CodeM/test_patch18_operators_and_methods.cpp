#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <sstream>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("operators", "[ops]") {
  Series a = pluss::series::full({4}, 2.0, ScalarType::Float32);
  Series b = pluss::series::full({4}, 3.0, ScalarType::Float32);

  Series c = a + b;
  auto* cp = reinterpret_cast<const float*>(c.raw_data());
  REQUIRE(cp[0] == 5.0f);

  Series n0 = -a;
  auto* n0p = reinterpret_cast<const float*>(n0.raw_data());
  REQUIRE(n0p[0] == -2.0f);

  Series n1 = -(a + 4.0);
  auto* n1p = reinterpret_cast<const float*>(n1.raw_data());
  REQUIRE(n1p[0] == -6.0f);

  a += b;
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 5.0f);

  Series m = (a > b);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
}

TEST_CASE("bitwise symbols compose boolean masks and integer bits", "[ops][bitwise]") {
  const Series lhs_mask = pluss::make_series({true, true, false, false});
  const Series rhs_mask = pluss::make_series({true, false, true, false});

  REQUIRE((lhs_mask & rhs_mask).items<bool>() ==
          std::vector<bool>{true, false, false, false});
  REQUIRE((lhs_mask | rhs_mask).items<bool>() ==
          std::vector<bool>{true, true, true, false});
  REQUIRE((~lhs_mask).items<bool>() ==
          std::vector<bool>{false, false, true, true});
  REQUIRE((~lhs_mask).items<bool>() == lhs_mask.logical_not().items<bool>());

  const Series lhs_bits = pluss::make_series({6, 5}, ScalarType::Int64);
  const Series rhs_bits = pluss::make_series({3, 10}, ScalarType::Int64);

  REQUIRE((lhs_bits & rhs_bits).items<std::int64_t>() ==
          std::vector<std::int64_t>{2, 0});
  REQUIRE((lhs_bits | rhs_bits).items<std::int64_t>() ==
          std::vector<std::int64_t>{7, 15});
  REQUIRE((~lhs_bits).items<std::int64_t>() ==
          std::vector<std::int64_t>{~std::int64_t{6}, ~std::int64_t{5}});
}

TEST_CASE("ostream", "[print]") {
  Series a = pluss::series::arange(0, 5, 1, ScalarType::Int64);
  std::ostringstream oss;
  oss << a;
  REQUIRE(oss.str().find("pluss::Series") != std::string::npos);
  REQUIRE(oss.str().find("dtype=") != std::string::npos);
}
