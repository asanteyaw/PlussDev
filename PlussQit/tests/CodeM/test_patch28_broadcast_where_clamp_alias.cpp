#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("broadcast where + clamp alias", "[where][broadcast][clamp]") {
  Series a = pluss::arange(0, 6, 1, ScalarType::Int64).view({2,3});
  Series b = pluss::full({1,3}, 100.0, ScalarType::Int64); // broadcast over dim0
  Series m = pluss::zeros({2,1}, ScalarType::Bool);        // broadcast over dim1
  m = m.index_put({pluss::full({1}, 1, ScalarType::Int64), pluss::full({1}, 0, ScalarType::Int64)},
                  pluss::full({1}, 1, ScalarType::Bool), false);

  auto wv = a.where(m, b);
  REQUIRE(wv.sizes()[0] == 2);
  REQUIRE(wv.sizes()[1] == 3);
  auto* wp = reinterpret_cast<const std::int64_t*>(wv.raw_data());
  REQUIRE(wp[0] == 0);    // row0 uses a
  REQUIRE(wp[3] == 100);  // row1 uses b due to mask

  auto c = pluss::full({3}, 2.0, ScalarType::Float32).clamp(3.0, 4.0);
  auto* cp = reinterpret_cast<const float*>(c.raw_data());
  REQUIRE(cp[0] == 3.0f);

  auto ws = a.where(m, 7.0);
  auto* wsp = reinterpret_cast<const std::int64_t*>(ws.raw_data());
  REQUIRE(wsp[3] == 7);
}
