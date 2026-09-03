#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("reshape and expand", "[shape]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto r = a.reshape({3,2});
  REQUIRE(r.sizes()[0] == 3);
  REQUIRE(r.sizes()[1] == 2);

  auto e = a.expand(std::vector<std::int64_t>{2,3}, true); // no-op
  REQUIRE(e.sizes()[0] == 2);
  REQUIRE(e.sizes()[1] == 3);

  auto b = Series::zeros({1,3}, ScalarType::Float64);
  auto be = b.expand(std::vector<std::int64_t>{2,3}, true);
  REQUIRE(be.strides()[0] == 0);
}

TEST_CASE("expand_like and repeat_like", "[shape]") {
  const auto like = pluss::series::rand({2, 6}, ScalarType::Float32, 123);

  const auto scalar = pluss::make_series(std::vector<int>{7}, {1, 1});
  const auto expanded = scalar.expand_like(like);
  REQUIRE(expanded.sizes()[0] == 2);
  REQUIRE(expanded.sizes()[1] == 6);
  REQUIRE(expanded.scalar_type() == ScalarType::Int64);
  REQUIRE(expanded.to_vec<std::int64_t>() == std::vector<std::int64_t>{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});

  const auto base = pluss::make_series(std::vector<int>{1, 2, 3}, {3});
  const auto repeated = base.repeat_like(like);
  REQUIRE(repeated.sizes()[0] == 2);
  REQUIRE(repeated.sizes()[1] == 6);
  REQUIRE(repeated.scalar_type() == ScalarType::Int64);
  REQUIRE(repeated.to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3});

  const auto bad_like = pluss::series::rand({2, 5}, ScalarType::Float32, 11);
  REQUIRE_THROWS(base.repeat_like(bad_like));
}

TEST_CASE("cat and stack", "[cat]") {
  auto a = Series::zeros({2,2}, ScalarType::Float32);
  auto b = Series::zeros({2,2}, ScalarType::Float32);
  auto* ap = reinterpret_cast<float*>(a.raw_data());
  auto* bp = reinterpret_cast<float*>(b.raw_data());
  for (int i=0;i<4;++i) ap[i] = float(i+1);
  for (int i=0;i<4;++i) bp[i] = float(10+i);

  auto c = a.unite({b}, 0);
  REQUIRE(c.sizes()[0] == 4);
  REQUIRE(c.sizes()[1] == 2);

  auto s = a.unsqueeze(0).unite({b.unsqueeze(0)}, 0);
  REQUIRE(s.sizes()[0] == 2);
  REQUIRE(s.sizes()[1] == 2);
  REQUIRE(s.sizes()[2] == 2);
}

TEST_CASE("unary math + reductions", "[math]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (int i=0;i<6;++i) ap[i] = (double)(i+1);

  auto ex = a.exp();
  REQUIRE(ex.numel() == 6);

  auto m = a.mean(1, false);
  REQUIRE(m.sizes()[0] == 2);
  auto* mp = reinterpret_cast<const double*>(m.raw_data());
  REQUIRE(mp[0] == Catch::Approx((1+2+3)/3.0));
  REQUIRE(mp[1] == Catch::Approx((4+5+6)/3.0));

  auto am = a.amax(1, false);
  auto* amx = reinterpret_cast<const double*>(am.raw_data());
  REQUIRE(amx[0] == 3.0);
  REQUIRE(amx[1] == 6.0);

  auto ag = a.argmax(1, false);
  auto* agp = reinterpret_cast<const std::int64_t*>(ag.raw_data());
  REQUIRE(agp[0] == 2);
  REQUIRE(agp[1] == 2);
}

TEST_CASE("root rearrangement methods preserve their documented axes", "[shape]") {
  auto table = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
  REQUIRE(table.flip(0).to_vec<std::int64_t>() == std::vector<std::int64_t>{4, 5, 6, 1, 2, 3});
  REQUIRE(table.fliplr().to_vec<std::int64_t>() == std::vector<std::int64_t>{3, 2, 1, 6, 5, 4});
  REQUIRE(table.roll(2).to_vec<std::int64_t>() == std::vector<std::int64_t>{5, 6, 1, 2, 3, 4});
  REQUIRE(table.roll(-1, 1).to_vec<std::int64_t>() == std::vector<std::int64_t>{2, 3, 1, 5, 6, 4});
  REQUIRE(table.rot90().to_vec<std::int64_t>() == std::vector<std::int64_t>{3, 6, 2, 5, 1, 4});
  REQUIRE_THROWS(table.roll({1, 2}));
}
