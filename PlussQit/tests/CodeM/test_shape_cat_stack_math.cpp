#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <complex>
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
  // std::vector<int> leaves are int32; expand_like and repeat_like keep the dtype.
  REQUIRE(expanded.scalar_type() == ScalarType::Int32);
  REQUIRE(expanded.to_vec<std::int32_t>() == std::vector<std::int32_t>{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});

  const auto base = pluss::make_series(std::vector<int>{1, 2, 3}, {3});
  const auto repeated = base.repeat_like(like);
  REQUIRE(repeated.sizes()[0] == 2);
  REQUIRE(repeated.sizes()[1] == 6);
  REQUIRE(repeated.scalar_type() == ScalarType::Int32);
  REQUIRE(repeated.to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3});

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

namespace {

// Independent reference: out[j] = in[(j - shift) mod n] through index_select.
Series roll_reference(const Series &x, std::int64_t shift, std::int64_t dim) {
  const std::int64_t n = x.size(dim);
  if (n <= 1) return x;
  std::vector<std::int64_t> index(static_cast<std::size_t>(n));
  for (std::int64_t j = 0; j < n; ++j)
    index[static_cast<std::size_t>(j)] = (((j - shift) % n) + n) % n;
  return x.index_select(
      dim, pluss::make_series(index, {n}).to(ScalarType::Int64));
}

} // namespace

TEST_CASE("roll matches an index_select reference across shapes, dims, shifts and dtypes", "[shape][roll]") {
  const std::vector<std::vector<std::int64_t>> shapes{{5}, {3, 4}, {2, 3, 4}, {4, 1, 3}, {2, 0, 3}};
  for (const auto &shape : shapes) {
    std::int64_t numel = 1;
    for (const auto extent : shape) numel *= extent;
    std::vector<double> values(static_cast<std::size_t>(numel));
    for (std::size_t i = 0; i < values.size(); ++i) values[i] = static_cast<double>(i) * 0.5 - 3.0;
    const Series base = pluss::make_series(values, shape).to(ScalarType::Float64);
    for (std::int64_t dim = -static_cast<std::int64_t>(shape.size());
         dim < static_cast<std::int64_t>(shape.size()); ++dim) {
      const std::int64_t axis = dim < 0 ? dim + static_cast<std::int64_t>(shape.size()) : dim;
      for (const std::int64_t shift : {0, 1, 2, -1, -3, 7, 100, -100}) {
        const Series got = base.roll(shift, dim);
        const Series want = roll_reference(base, shift, axis);
        REQUIRE(got.sizes() == base.sizes());
        REQUIRE(got.equal(want));
      }
    }
  }

  // Non-contiguous input, integer, boolean and complex payloads (byte copies).
  const Series transposed = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3})
                                .to(ScalarType::Float64).transpose(0, 1);
  REQUIRE(!transposed.is_contiguous());
  REQUIRE(transposed.roll(1, 0).equal(roll_reference(transposed, 1, 0)));
  REQUIRE(transposed.roll(-1, 1).equal(roll_reference(transposed, -1, 1)));
  const Series ints = pluss::make_series(std::vector<std::int32_t>{1, 2, 3, 4}, {4});
  REQUIRE(ints.roll(1, 0).to_vec<std::int32_t>() == std::vector<std::int32_t>{4, 1, 2, 3});
  const Series flags = pluss::make_series(std::vector<double>{1, 0, 0}, {3}).to(ScalarType::Bool);
  REQUIRE(flags.roll(1, 0).to(ScalarType::Float64).to_vec<double>() == std::vector<double>{0, 1, 0});
  const Series z = pluss::make_series(std::vector<std::complex<double>>{{1, 2}, {3, 4}, {5, 6}}, {3});
  REQUIRE(z.roll(1, 0).to_vec<std::complex<double>>() ==
          std::vector<std::complex<double>>{{5, 6}, {1, 2}, {3, 4}});
}

TEST_CASE("roll reverse routes the sensitivity back by the opposite shift", "[shape][roll][autodiff]") {
  Series x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5}, {5}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series weights = pluss::make_series(std::vector<double>{10, 20, 30, 40, 50}, {5}).to(ScalarType::Float64);
  x.roll(2, 0).mul(weights).sum().reverse();
  // out[j] = x[(j-2) mod 5] => d/dx[i] = weights[(i+2) mod 5]
  REQUIRE(x.sensitivity().to_vec<double>() == std::vector<double>{30, 40, 50, 10, 20});
}
