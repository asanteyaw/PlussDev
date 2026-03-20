#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("basic arithmetic broadcast", "[arith]") {
  auto a = Series::full({2,2}, 2.0, ScalarType::Float32);
  auto b = Series::full({1,2}, 3.0, ScalarType::Float32);
  auto c = a.add(b);
  auto* cp = reinterpret_cast<const float*>(c.raw_data());
  REQUIRE(cp[0] == 5.0f);
  REQUIRE(cp[3] == 5.0f);

  auto d = c.mul(Series::full({}, 2.0, ScalarType::Float32));
  auto* dp = reinterpret_cast<const float*>(d.raw_data());
  REQUIRE(dp[1] == 10.0f);
}

TEST_CASE("abs/sign/int arithmetic", "[arith]") {
  auto a = Series::zeros({3}, ScalarType::Int64);
  auto* ap = reinterpret_cast<std::int64_t*>(a.raw_data());
  ap[0] = -3; ap[1] = 0; ap[2] = 5;

  auto b = a.abs();
  auto* bp = reinterpret_cast<const std::int64_t*>(b.raw_data());
  REQUIRE(bp[0] == 3);

  auto s = a.sign();
  auto* sp = reinterpret_cast<const std::int64_t*>(s.raw_data());
  REQUIRE(sp[0] == -1);
  REQUIRE(sp[1] == 0);
  REQUIRE(sp[2] == 1);
}

TEST_CASE("scalar reductions sum/all/any", "[reduce]") {
  auto a = Series::zeros({4}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  ap[0]=1; ap[1]=2; ap[2]=3; ap[3]=4;

  auto s = a.sum();
  REQUIRE(s.dim() == 0);
  REQUIRE(*reinterpret_cast<const double*>(s.raw_data()) == 10.0);

  auto m = Series::zeros({3}, ScalarType::Bool);
  auto* mp = reinterpret_cast<std::uint8_t*>(m.raw_data());
  mp[0]=1; mp[1]=1; mp[2]=0;
  auto ms = m.sum();
  REQUIRE(ms.scalar_type() == ScalarType::Int64);
  REQUIRE(*reinterpret_cast<const std::int64_t*>(ms.raw_data()) == 2);
  REQUIRE(*reinterpret_cast<const std::uint8_t*>(m.all().raw_data()) == 0);
  REQUIRE(*reinterpret_cast<const std::uint8_t*>(m.any().raw_data()) == 1);
}

TEST_CASE("bool sum over dimension returns count tensor", "[reduce]") {
  auto m = Series::zeros({2,3}, ScalarType::Bool);
  auto* mp = reinterpret_cast<std::uint8_t*>(m.raw_data());
  // row 0: [1,0,1], row 1: [0,1,0]
  mp[0]=1; mp[1]=0; mp[2]=1;
  mp[3]=0; mp[4]=1; mp[5]=0;

  auto col = m.sum(0, false);
  REQUIRE(col.scalar_type() == ScalarType::Int64);
  auto* cp = reinterpret_cast<const std::int64_t*>(col.raw_data());
  REQUIRE(cp[0] == 1);
  REQUIRE(cp[1] == 1);
  REQUIRE(cp[2] == 1);

  auto row = m.sum(1, false);
  REQUIRE(row.scalar_type() == ScalarType::Int64);
  auto* rp = reinterpret_cast<const std::int64_t*>(row.raw_data());
  REQUIRE(rp[0] == 2);
  REQUIRE(rp[1] == 1);
}

TEST_CASE("fp16/bf16 arithmetic works (via compute in f32)", "[arith]") {
  auto a = Series::full({2}, 2.0, ScalarType::Float16);
  auto b = Series::full({2}, 3.0, ScalarType::Float16);
  auto c = a.add(b);
  REQUIRE(c.scalar_type() == ScalarType::Float16);

  auto d = c.to(ScalarType::BFloat16).clamp(0.0, 4.0);
  REQUIRE(d.scalar_type() == ScalarType::BFloat16);
}
