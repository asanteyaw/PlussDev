#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace {

std::vector<std::int64_t> dims(pluss::msharp::RangeView::Int values) {
  return {values.begin(), values.end()};
}

} // namespace

TEST_CASE("shape metadata distinguishes rank extent and logical layout", "[introspection]") {
  const auto scalar = pluss::make_series(7.0);
  REQUIRE(scalar.dim() == 0);
  REQUIRE(scalar.numel() == 1);
  REQUIRE(dims(scalar.strides()).empty());
  REQUIRE_THROWS(scalar.stride(0));

  const auto matrix = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  REQUIRE(matrix.dim() == 2);
  REQUIRE(matrix.numel() == 6);
  REQUIRE(dims(matrix.strides()) == std::vector<std::int64_t>{3, 1});
  REQUIRE(matrix.stride(0) == 3);
  REQUIRE(matrix.stride(-1) == 1);

  const auto transposed = matrix.transpose(0, 1);
  REQUIRE(dims(transposed.sizes()) == std::vector<std::int64_t>{3, 2});
  REQUIRE(dims(transposed.strides()) == std::vector<std::int64_t>{1, 3});
  REQUIRE_FALSE(transposed.is_contiguous());
  REQUIRE_THROWS(matrix.stride(2));

  const auto empty = pluss::Series::empty({2, 0, 4}, pluss::ScalarType::Float64);
  REQUIRE(empty.dim() == 3);
  REQUIRE(empty.numel() == 0);
}

TEST_CASE("dtype and platform predicates return native booleans", "[introspection]") {
  const auto real = pluss::Series::zeros({2}, pluss::ScalarType::Float32);
  const auto integer = pluss::Series::zeros({2}, pluss::ScalarType::Int32);
  const auto unsigned_integer = pluss::Series::zeros({2}, pluss::ScalarType::UInt32);
  const auto complex = pluss::Series::zeros({2}, pluss::ScalarType::Complex64);

  REQUIRE(real.dtype().scalar == pluss::ScalarType::Float32);
  REQUIRE(real.dtype().itemsize == 4);
  REQUIRE(real.is_floating_point());
  REQUIRE(real.is_signed());
  REQUIRE_FALSE(real.is_complex());
  REQUIRE(integer.is_signed());
  REQUIRE_FALSE(integer.is_floating_point());
  REQUIRE_FALSE(unsigned_integer.is_signed());
  REQUIRE(complex.is_complex());
  REQUIRE(complex.is_signed());

  REQUIRE(real.is_cpu());
  REQUIRE_FALSE(real.is_pinned());
  REQUIRE_FALSE(real.is_gpu());
  REQUIRE_FALSE(real.is_cuda());
  REQUIRE_FALSE(real.is_mps());
}

TEST_CASE("autodiff scalar and identity predicates inspect different state", "[introspection]") {
  auto leaf = pluss::make_series({1.0, 2.0});
  leaf.tracks_slope_(true);
  const auto result = leaf.square();
  REQUIRE(leaf.is_leaf());
  REQUIRE_FALSE(result.is_leaf());

  REQUIRE(pluss::make_series(3.0).is_nonzero());
  REQUIRE_FALSE(pluss::make_series(0.0).is_nonzero());
  REQUIRE(pluss::make_series({3.0}).is_nonzero());
  REQUIRE_THROWS(pluss::make_series({1.0, 2.0}).is_nonzero());

  const auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const auto alias = base;
  const auto view = base.transpose(0, 1);
  const auto copy = base.clone();
  REQUIRE(base.is_same_size(view));
  REQUIRE(base.is_set_to(alias));
  REQUIRE_FALSE(base.is_set_to(view));
  REQUIRE_FALSE(base.is_set_to(copy));
}

TEST_CASE("equal requires the same shape and exact element equality", "[introspection]") {
  const auto values = pluss::make_series({1, 2, 3});
  REQUIRE(values.equal(pluss::make_series({1, 2, 3})));
  REQUIRE(values.equal(pluss::make_series({1.0, 2.0, 3.0})));
  REQUIRE_FALSE(values.equal(pluss::make_series({{1, 2, 3}})));
  REQUIRE_FALSE(values.equal(pluss::make_series({1, 2, 4})));
  REQUIRE(pluss::Series::empty({0}, pluss::ScalarType::Float64)
              .equal(pluss::Series::empty({0}, pluss::ScalarType::Float64)));

  const double nan = std::numeric_limits<double>::quiet_NaN();
  REQUIRE_FALSE(pluss::make_series({nan}).equal(pluss::make_series({nan})));
}

TEST_CASE("isclose uses asymmetric tolerance and supports broadcasting", "[introspection]") {
  const auto a = pluss::make_series({9.0, 20.0}).reshape({2, 1});
  const auto b = pluss::make_series({{10.0, 20.0, 40.0}});
  const auto close = a.isclose(b, 0.1, 0.0);
  REQUIRE(dims(close.sizes()) == std::vector<std::int64_t>{2, 3});
  REQUIRE(close.host<bool>() ==
          std::vector<bool>{true, false, false, false, true, false});
  REQUIRE(pluss::make_series({9.0}).isclose(pluss::make_series({10.0}), 0.1, 0.0)
              .host<bool>().front());
  REQUIRE_FALSE(pluss::make_series({10.0})
                    .isclose(pluss::make_series({9.0}), 0.1, 0.0)
                    .host<bool>().front());
}

TEST_CASE("isclose treats exact infinities and optional NaNs correctly", "[introspection]") {
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const auto a = pluss::make_series({inf, -inf, nan, 1.0});
  const auto b = pluss::make_series({inf, inf, nan, 1.000001});

  REQUIRE(a.isclose(b).host<bool>() ==
          std::vector<bool>{true, false, false, true});
  REQUIRE(a.isclose(b, 1e-5, 1e-8, true).host<bool>() ==
          std::vector<bool>{true, false, true, true});
  REQUIRE_FALSE(a.allclose(b));
  REQUIRE_FALSE(a.allclose(b, 1e-5, 1e-8, true));
  REQUIRE(pluss::make_series({inf, nan})
              .allclose(pluss::make_series({inf, nan}), 1e-5, 1e-8, true));
}

TEST_CASE("closeness validates dtype and tolerance contracts", "[introspection]") {
  const auto floats = pluss::make_series({1.0, 2.0});
  const auto integers = pluss::make_series({1, 2});
  REQUIRE_THROWS(floats.isclose(integers));
  REQUIRE_THROWS(floats.allclose(integers));
  REQUIRE_THROWS(floats.isclose(floats, -1e-5, 1e-8));
  REQUIRE_THROWS(floats.isclose(floats, 1e-5, -1e-8));
}
