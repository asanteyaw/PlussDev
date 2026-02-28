#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("type promotion basic lattice", "[dtype]") {
  REQUIRE(promote_types(ScalarType::Float32, ScalarType::Float64) == ScalarType::Float64);
  REQUIRE(promote_types(ScalarType::Int32, ScalarType::Float32) == ScalarType::Float32);
  REQUIRE(promote_types(ScalarType::Bool, ScalarType::Int8) == ScalarType::Int16);
  REQUIRE(promote_types(ScalarType::UInt8, ScalarType::Int64) == ScalarType::Int64);
}

TEST_CASE("scatter_add accumulates", "[indexing]") {
  auto x = Series::zeros({2,3}, ScalarType::Float64);
  auto idx = Series::zeros({2,2}, ScalarType::Int64);
  auto src = Series::zeros({2,2}, ScalarType::Float64);

  // idx: place both values into col 1 for each row (accumulate)
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  ip[0]=1; ip[1]=1; ip[2]=1; ip[3]=1;

  auto* sp = reinterpret_cast<double*>(src.raw_data());
  sp[0]=1; sp[1]=2; sp[2]=3; sp[3]=4;

  auto y = scatter_add(x, 1, idx, src);
  auto* yp = reinterpret_cast<const double*>(y.raw_data());
  // row0 col1 = 1+2 = 3 ; row1 col1 = 3+4 = 7
  REQUIRE(yp[1] == 3.0);
  REQUIRE(yp[4] == 7.0);
}
