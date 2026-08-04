#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <array>
#include <complex>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("index_put_ accumulate supports float16", "[indexing][accumulate]") {
  auto x = pluss::series::zeros({4}, pluss::config({{"dtype","float16"}}));
  auto idx = pluss::make_series(std::vector<std::int64_t>{1,1,2}, {3});
  auto v  = pluss::make_series(std::vector<float>{1,2,3}, {3}).to(pluss::ScalarType::Float16);
  x.index_put_({idx}, v, true);
  REQUIRE(x.numel() == 4);
}

TEST_CASE("index_put_ accumulate supports complex64", "[indexing][accumulate][complex]") {
  using c64 = std::complex<float>;
  std::vector<c64> base = {c64(0,0), c64(0,0), c64(0,0)};
  auto x = pluss::make_series(base, {3}, pluss::config({{"dtype","complex64"}}));
  auto idx = pluss::make_series(std::vector<std::int64_t>{1,1}, {2});
  std::vector<c64> vv = {c64(1,2), c64(3,4)};
  auto v = pluss::make_series(vv, {2}, pluss::config({{"dtype","complex64"}}));
  const std::array<Series, 1> indices{{idx}};
  x.index_put_(pluss::msharp::RangeView::Of<Series>(indices), v, true);
  REQUIRE(x.numel() == 3);
  const auto got = x.to_vec<c64>();
  REQUIRE(got[0] == c64(0, 0));
  REQUIRE(got[1] == c64(4, 6));
  REQUIRE(got[2] == c64(0, 0));
}

TEST_CASE("index_put_ accumulate supports complex128", "[indexing][accumulate][complex]") {
  using c128 = std::complex<double>;
  std::vector<c128> base = {c128(0,0), c128(0,0), c128(0,0)};
  auto x = pluss::make_series(base, {3}, pluss::config({{"dtype","complex128"}}));
  auto idx = pluss::make_series(std::vector<std::int64_t>{2,2}, {2});
  std::vector<c128> vv = {c128(1.5, -2.0), c128(3.25, 4.5)};
  auto v = pluss::make_series(vv, {2}, pluss::config({{"dtype","complex128"}}));
  const std::array<Series, 1> indices{{idx}};
  x.index_put_(pluss::msharp::RangeView::Of<Series>(indices), v, true);
  REQUIRE(x.numel() == 3);
  const auto got = x.to_vec<c128>();
  REQUIRE(got[0] == c128(0, 0));
  REQUIRE(got[1] == c128(0, 0));
  REQUIRE(got[2] == c128(4.75, 2.5));
}

TEST_CASE("tanh/softplus run", "[activation]") {
  auto a = pluss::make_series(std::vector<float>{-1,0,1,2}, {2,2});
  auto t = a.tanh();
  auto s = a.softplus();
  REQUIRE(t.numel() == 4);
  REQUIRE(s.numel() == 4);
}
