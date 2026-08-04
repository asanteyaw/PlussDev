#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("broadcastable full-tensor bool mask in indices vector", "[indexing]") {
  pluss::Series a = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64).view(std::vector<std::int64_t>{3,4});
  // mask shape [1,4] broadcast to [3,4]
  pluss::Series m = pluss::series::full({1,4}, 0, pluss::ScalarType::Bool);
  auto* mp = reinterpret_cast<bool*>(m.raw_data());
  mp[1] = true; mp[3] = true; // select columns 1 and 3 across all rows

  std::vector<pluss::Series> idx = { m }; // old path also supported; still valid
  pluss::Series v = pluss::series::full({6}, 99, pluss::ScalarType::Int64); // nnz = 6
  pluss::Series out = pluss::index_put(a, idx, v, false);
  REQUIRE(out.numel() == a.numel());
}

TEST_CASE("scatter_reduce supports uint/int8/int16/half", "[scatter][reduce]") {
  pluss::Series self = pluss::series::zeros({2,3}, pluss::ScalarType::UInt8);
  pluss::Series src  = pluss::series::full({2,3}, 1, pluss::ScalarType::UInt8);
  pluss::Series idx  = pluss::series::full({2,3}, 0, pluss::ScalarType::Int64);
  pluss::Series out = self.scatter_reduce_sum(1, idx, src, false, 0.0);
  const auto* p = reinterpret_cast<const std::uint8_t*>(out.raw_data());
  REQUIRE(p[0] == 3);
  REQUIRE(p[3] == 3);

  pluss::Series selfh = pluss::series::zeros({2,3}, pluss::ScalarType::Float16);
  pluss::Series srch  = pluss::series::full({2,3}, 1.0, pluss::ScalarType::Float16);
  pluss::Series outh  = selfh.scatter_reduce_sum(1, idx, srch, false, 0.0);
  REQUIRE(outh.scalar_type() == pluss::ScalarType::Float16);
}

TEST_CASE("scatter_reduce complex sum works, amax throws", "[scatter][complex]") {
  pluss::Series self = pluss::series::full({2,2}, 0.0, pluss::ScalarType::Complex64);
  pluss::Series src  = pluss::series::full({2,2}, 1.0, pluss::ScalarType::Complex64);
  pluss::Series idx  = pluss::series::full({2,2}, 0, pluss::ScalarType::Int64);
  pluss::Series out = self.scatter_reduce_sum(1, idx, src, false, 0.0);
  REQUIRE(out.scalar_type() == pluss::ScalarType::Complex64);
  REQUIRE_THROWS_AS(self.scatter_reduce_amax(1, idx, src, false, 0.0), pluss::Error);
}

TEST_CASE("printing is stable for small tensors", "[printing]") {
  pluss::Series x = pluss::series::arange(0, 6, 1, pluss::ScalarType::Int64).view(std::vector<std::int64_t>{2,3});
  auto s = pluss::codem::to_string(x);
  REQUIRE(s.find("Series(shape=[2,3]") != std::string::npos);
  REQUIRE(s.find("[[") != std::string::npos);
}
