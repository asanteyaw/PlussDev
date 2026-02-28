#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <vector>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("make_series type deduction", "[factory]") {
  std::vector<std::int64_t> v = {1,2,3,4};
  Series x = pluss::make_series(v, {2,2}); // no explicit <T>
  auto* p = reinterpret_cast<const std::int64_t*>(x.raw_data());
  REQUIRE(p[2] == 3);
}
