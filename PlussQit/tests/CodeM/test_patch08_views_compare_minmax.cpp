#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("permute/transpose/unsqueeze/squeeze views", "[view]") {
  auto a = Series::zeros({2,3,4}, ScalarType::Float64);
  auto p = a.permute({2,1,0});
  REQUIRE(p.sizes()[0] == 4);
  REQUIRE(p.sizes()[1] == 3);
  REQUIRE(p.sizes()[2] == 2);

  auto t = a.transpose(0,2);
  REQUIRE(t.sizes()[0] == 4);
  REQUIRE(t.sizes()[2] == 2);

  auto u = a.unsqueeze(1);
  REQUIRE(u.dim() == 4);
  REQUIRE(u.sizes()[1] == 1);

  auto s = u.squeeze(1);
  REQUIRE(s.dim() == 3);
  REQUIRE(s.sizes()[0] == 2);
}

TEST_CASE("comparisons produce bool", "[cmp]") {
  auto a = Series::zeros({2,2}, ScalarType::Int64);
  auto b = Series::zeros({2,2}, ScalarType::Int64);
  auto* ap = reinterpret_cast<std::int64_t*>(a.raw_data());
  auto* bp = reinterpret_cast<std::int64_t*>(b.raw_data());
  ap[0]=1; ap[1]=2; ap[2]=3; ap[3]=4;
  bp[0]=1; bp[1]=1; bp[2]=4; bp[3]=4;

  auto m = eq(a,b);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
  auto* mp = reinterpret_cast<const std::uint8_t*>(m.raw_data());
  REQUIRE(mp[0] == 1);
  REQUIRE(mp[1] == 0);
}

TEST_CASE("minimum/maximum/clamp", "[minmax]") {
  auto a = Series::zeros({3}, ScalarType::Float32);
  auto b = Series::zeros({3}, ScalarType::Float32);
  auto* ap = reinterpret_cast<float*>(a.raw_data());
  auto* bp = reinterpret_cast<float*>(b.raw_data());
  ap[0]=1; ap[1]=5; ap[2]=3;
  bp[0]=2; bp[1]=4; bp[2]=3;

  auto mn = minimum(a,b);
  auto mx = maximum(a,b);
  auto* mnp = reinterpret_cast<const float*>(mn.raw_data());
  auto* mxp = reinterpret_cast<const float*>(mx.raw_data());
  REQUIRE(mnp[0] == 1.0f);
  REQUIRE(mxp[1] == 5.0f);

  auto c = clamp(a, 2.0, 4.0);
  auto* cp = reinterpret_cast<const float*>(c.raw_data());
  REQUIRE(cp[0] == 2.0f);
  REQUIRE(cp[1] == 4.0f);
  REQUIRE(cp[2] == 3.0f);
}
