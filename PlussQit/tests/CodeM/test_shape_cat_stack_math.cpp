#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("reshape and expand", "[shape]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto r = a.reshape({3,2});
  REQUIRE(r.sizes()[0] == 3);
  REQUIRE(r.sizes()[1] == 2);

  auto e = a.expand({2,3}); // no-op
  REQUIRE(e.sizes()[0] == 2);
  REQUIRE(e.sizes()[1] == 3);

  auto b = Series::zeros({1,3}, ScalarType::Float64);
  auto be = b.expand({2,3});
  REQUIRE(be.strides()[0] == 0);
}

TEST_CASE("cat and stack", "[cat]") {
  auto a = Series::zeros({2,2}, ScalarType::Float32);
  auto b = Series::zeros({2,2}, ScalarType::Float32);
  auto* ap = reinterpret_cast<float*>(a.raw_data());
  auto* bp = reinterpret_cast<float*>(b.raw_data());
  for (int i=0;i<4;++i) ap[i] = float(i+1);
  for (int i=0;i<4;++i) bp[i] = float(10+i);

  auto c = cat(std::vector<Series>{a,b}, 0);
  REQUIRE(c.sizes()[0] == 4);
  REQUIRE(c.sizes()[1] == 2);

  auto s = stack(std::vector<Series>{a,b}, 0);
  REQUIRE(s.sizes()[0] == 2);
  REQUIRE(s.sizes()[1] == 2);
  REQUIRE(s.sizes()[2] == 2);
}

TEST_CASE("unary math + reductions", "[math]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = (double)(i+1);

  auto ex = exp(a);
  REQUIRE(ex.numel() == 6);

  auto m = mean(a, 1, false);
  REQUIRE(m.sizes()[0] == 2);
  auto* mp = reinterpret_cast<const double*>(m.raw_data());
  REQUIRE(mp[0] == Approx((1+2+3)/3.0));
  REQUIRE(mp[1] == Approx((4+5+6)/3.0));

  auto am = amax(a, 1, false);
  auto* amx = reinterpret_cast<const double*>(am.raw_data());
  REQUIRE(amx[0] == 3.0);
  REQUIRE(amx[1] == 6.0);

  auto ag = argmax(a, 1, false);
  auto* agp = reinterpret_cast<const std::int64_t*>(ag.raw_data());
  REQUIRE(agp[0] == 2);
  REQUIRE(agp[1] == 2);
}
