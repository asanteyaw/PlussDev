#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("where + clamp + cum", "[where][clamp][cum]") {
  Series a = pluss::arange(0, 6, 1, ScalarType::Int64).view({2,3});
  Series b = pluss::full({2,3}, 100.0, ScalarType::Int64);
  Series m = pluss::zeros({2,3}, ScalarType::Bool);
  m = m.index_put({pluss::full({1}, 0, ScalarType::Int64), pluss::full({1}, 1, ScalarType::Int64)},
                  pluss::full({1}, 1, ScalarType::Bool), false);

  auto wv = a.where(m, b);
  auto* wp = reinterpret_cast<const std::int64_t*>(wv.raw_data());
  REQUIRE(wp[1] == 100);

  auto cf = pluss::full({3}, 2.0, ScalarType::Float32).clamp_min(3.0);
  auto* cp = reinterpret_cast<const float*>(cf.raw_data());
  REQUIRE(cp[0] == 3.0f);

  Series x = pluss::arange(1, 5, 1, ScalarType::Int64);
  auto cs = x.cumsum(0);
  auto* csp = reinterpret_cast<const std::int64_t*>(cs.raw_data());
  REQUIRE(csp[3] == 10);

  auto cp2 = x.cumprod(0);
  auto* cpp = reinterpret_cast<const std::int64_t*>(cp2.raw_data());
  REQUIRE(cpp[3] == 24);
}
