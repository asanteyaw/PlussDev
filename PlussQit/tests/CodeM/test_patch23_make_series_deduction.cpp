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

TEST_CASE("make_series accepts lvalue Config option", "[factory][config]") {
  pluss::Config cfg = pluss::Config{}
                          .with_dtype(pluss::ScalarType::Float64)
                          .with_platform(pluss::pCPU);

  std::vector<double> v = {1.0, 2.0, 3.0, 4.0};
  Series x = pluss::make_series(v, {2, 2}, cfg);

  REQUIRE(x.scalar_type() == pluss::ScalarType::Float64);
  REQUIRE(x.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({4}).select(0, 2).item<double>() == 3.0);
}
