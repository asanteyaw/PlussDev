#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("where broadcast + type promotion", "[indexing]") {
  auto cond = Series::zeros({2,1}, ScalarType::Bool);
  auto* cp = reinterpret_cast<std::uint8_t*>(cond.raw_data());
  cp[0] = 1; cp[1] = 0; // [[T],[F]]

  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto b = Series::zeros({1,3}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = 10.0 + i;
  auto* bp = reinterpret_cast<double*>(b.raw_data());
  bp[0]=1; bp[1]=2; bp[2]=3;

  auto out = where(cond, a, b);
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 3);
  auto* op = reinterpret_cast<const double*>(out.raw_data());
  // row0 from a, row1 from b
  REQUIRE(op[0] == 10.0);
  REQUIRE(op[1] == 11.0);
  REQUIRE(op[2] == 12.0);
  REQUIRE(op[3] == 1.0);
  REQUIRE(op[4] == 2.0);
  REQUIRE(op[5] == 3.0);
}

TEST_CASE("masked_fill and nonzero", "[indexing]") {
  auto a = Series::zeros({2,3}, ScalarType::Float32);
  auto* ap = reinterpret_cast<float*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = float(i);

  auto m = Series::zeros({2,3}, ScalarType::Bool);
  auto* mp = reinterpret_cast<std::uint8_t*>(m.raw_data());
  mp[0]=1; mp[5]=1;

  auto b = masked_fill(a, m, 99.0);
  auto* bp = reinterpret_cast<const float*>(b.raw_data());
  REQUIRE(bp[0] == 99.0f);
  REQUIRE(bp[5] == 99.0f);

  auto nz = nonzero(m);
  REQUIRE(nz.sizes()[1] == 2);
  REQUIRE(nz.sizes()[0] == 2);
  auto* ip = reinterpret_cast<const std::int64_t*>(nz.raw_data());
  // first true at (0,0), second at (1,2)
  REQUIRE(ip[0] == 0); REQUIRE(ip[1] == 0);
  REQUIRE(ip[2] == 1); REQUIRE(ip[3] == 2);
}

TEST_CASE("take/put", "[indexing]") {
  auto a = Series::zeros({6}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = double(i+1);

  auto idx = Series::zeros({3}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  ip[0] = 0; ip[1] = 3; ip[2] = -1;

  auto t = a.take(idx);
  auto* tp = reinterpret_cast<const double*>(t.raw_data());
  REQUIRE(tp[0] == 1.0);
  REQUIRE(tp[1] == 4.0);
  REQUIRE(tp[2] == 6.0);

  auto src = Series::zeros({3}, ScalarType::Float64);
  auto* sp = reinterpret_cast<double*>(src.raw_data());
  sp[0]=10; sp[1]=20; sp[2]=30;

  auto p = a.put(idx, src, false);
  auto* pp = reinterpret_cast<const double*>(p.raw_data());
  REQUIRE(pp[0] == 10.0);
  REQUIRE(pp[3] == 20.0);
  REQUIRE(pp[5] == 30.0);
}
