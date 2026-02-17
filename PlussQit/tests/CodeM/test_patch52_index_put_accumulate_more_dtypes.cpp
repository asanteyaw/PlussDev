#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <complex>

TEST_CASE("index_put_ accumulate supports float16", "[indexing][accumulate]") {
  auto x = pluss::zeros({4}, {{"dtype","float16"}});
  auto idx = pluss::value(std::vector<std::int64_t>{1,1,2}, {3});
  auto v  = pluss::value(std::vector<float>{1,2,3}, {3}).to(pluss::ScalarType::Float16);
  x.index_put_({idx}, v, true);
  REQUIRE(x.numel() == 4);
}

TEST_CASE("index_put_ accumulate supports complex64", "[indexing][accumulate][complex]") {
  using c64 = std::complex<float>;
  std::vector<c64> base = {c64(0,0), c64(0,0), c64(0,0)};
  auto x = pluss::make_series(base, {3}, {{"dtype","complex64"}});
  auto idx = pluss::value(std::vector<std::int64_t>{1,1}, {2});
  std::vector<c64> vv = {c64(1,2), c64(3,4)};
  auto v = pluss::make_series(vv, {2}, {{"dtype","complex64"}});
  x.index_put_({idx}, v, true);
  REQUIRE(x.numel() == 3);
}

TEST_CASE("tanh/softplus run", "[activation]") {
  auto a = pluss::value(std::vector<float>{-1,0,1,2}, {2,2});
  auto t = a.tanh();
  auto s = a.softplus();
  REQUIRE(t.numel() == 4);
  REQUIRE(s.numel() == 4);
}
