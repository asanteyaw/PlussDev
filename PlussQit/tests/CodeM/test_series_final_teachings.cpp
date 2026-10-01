#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <complex>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

using Catch::Approx;
using Complex = std::complex<double>;

}  // namespace

TEST_CASE("stft covers real, complex, packed, batch, and reverse contracts",
          "[series][stft][final-teachings]") {
  auto signal = pluss::make_series({1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
  auto spectrum = signal.stft(4, 2, 4, pluss::series::undefined(), true,
                              "reflect", false, true, true);
  REQUIRE(spectrum.sizes() == std::vector<std::int64_t>{3, 5});
  REQUIRE(spectrum.scalar_type() == pluss::ScalarType::Complex128);

  const auto packed = signal.stft(4, 2, 4, pluss::series::undefined(), true,
                                  "reflect", false, true, false);
  REQUIRE(packed.sizes() == std::vector<std::int64_t>{3, 5, 2});
  REQUIRE(packed.scalar_type() == pluss::ScalarType::Float64);

  const auto batch = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                                         {4.0, 3.0, 2.0, 1.0}});
  REQUIRE(batch.stft(4, 2, 4, pluss::series::undefined(), false,
                     "constant", false, true, true).sizes() ==
          std::vector<std::int64_t>{2, 3, 1});

  const auto complex = pluss::make_series(
      std::vector<Complex>{{1.0, 1.0}, {2.0, 0.0}, {3.0, -1.0}, {4.0, 0.0}}, {4});
  const auto complex_spectrum = complex.stft(
      4, 4, 4, pluss::series::undefined(), false, "constant", false, false, true);
  REQUIRE(complex_spectrum.sizes() == std::vector<std::int64_t>{4, 1});
  REQUIRE(complex_spectrum.to_vec<Complex>()[0] == Complex{10.0, 0.0});

  signal.tracks_slope_(true);
  signal.stft(4, 2, 4, pluss::series::undefined(), true,
              "reflect", true, true, true).abs().sum().reverse();
  REQUIRE(signal.has_sensitivity());
  REQUIRE(signal.sensitivity().sizes() == signal.sizes());

  REQUIRE_THROWS(pluss::make_series({1, 2, 3, 4}).stft(4));
  REQUIRE_THROWS(signal.stft(2));
  REQUIRE_THROWS(complex.stft(4, 4, 4, pluss::series::undefined(), false,
                              "constant", false, true, true));
  REQUIRE_THROWS(signal.stft(4, 2, 3, pluss::make_series({1.0, 1.0}), true,
                             "reflect", false, true, true));
}

TEST_CASE("sum_to_size reverses broadcasting and remains differentiable",
          "[series][sum_to_size][final-teachings]") {
  auto x = pluss::series::arange(1.0, 25.0, 1.0, pluss::ScalarType::Float64)
               .reshape({2, 3, 4});
  const auto reduced = x.sum_to_size({1, 3, 1});
  REQUIRE(reduced.sizes() == std::vector<std::int64_t>{1, 3, 1});
  REQUIRE(reduced.to_vec<double>() == std::vector<double>{68.0, 100.0, 132.0});
  REQUIRE(x.sum_to_size({}).item<double>() == Approx(300.0));
  REQUIRE(x.sum_to_size({2, 3, 4}).allclose(x));

  x.tracks_slope_(true);
  x.sum_to_size({1, 3, 1}).sum().reverse();
  REQUIRE(x.sensitivity().allclose(pluss::series::ones(
      {2, 3, 4}, pluss::ScalarType::Float64)));

  REQUIRE_THROWS(x.sum_to_size({2, 2, 4}));
  REQUIRE_THROWS(x.sum_to_size({1, 2, 3, 4}));
}

TEST_CASE("threshold preserves exact branch and in-place contracts",
          "[series][threshold][threshold_][final-teachings]") {
  auto x = pluss::make_series({-1.0, 0.0, 0.5, 1.0});
  REQUIRE(x.threshold(0.5, -9.0).to_vec<double>() ==
          std::vector<double>{-9.0, -9.0, -9.0, 1.0});
  REQUIRE(pluss::make_series({-1, 0, 1}).threshold(-0.5, 9.5).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{9, 0, 1});

  auto changed = x.clone();
  auto* address = &changed;
  auto& returned = changed.threshold_(0.0, 7.0);
  REQUIRE(&returned == address);
  REQUIRE(changed.to_vec<double>() == std::vector<double>{7.0, 7.0, 0.5, 1.0});

  x.tracks_slope_(true);
  x.threshold(0.5, -9.0).sum().reverse();
  REQUIRE(x.sensitivity().to_vec<double>() ==
          std::vector<double>{0.0, 0.0, 0.0, 1.0});

  REQUIRE_THROWS(pluss::make_series({true, false}).threshold(0.0, 1.0));
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1})
                     .threshold(0.0, 1.0));
}

TEST_CASE("unflatten supports inferred, negative, empty, and reverse paths",
          "[series][unflatten][final-teachings]") {
  auto x = pluss::series::arange(0.0, 24.0, 1.0, pluss::ScalarType::Float64)
               .reshape({2, 12});
  REQUIRE(x.unflatten(1, {3, 4}).sizes() == std::vector<std::int64_t>{2, 3, 4});
  REQUIRE(x.unflatten(-1, {2, -1}).sizes() == std::vector<std::int64_t>{2, 2, 6});
  REQUIRE(pluss::series::empty({0}, pluss::ScalarType::Float64)
              .unflatten(0, {2, -1}).sizes() == std::vector<std::int64_t>{2, 0});

  x.tracks_slope_(true);
  x.unflatten(1, {3, 4}).sum().reverse();
  REQUIRE(x.sensitivity().allclose(pluss::series::ones(
      {2, 12}, pluss::ScalarType::Float64)));

  REQUIRE_THROWS(x.unflatten(1, {}));
  REQUIRE_THROWS(x.unflatten(1, {-1, -1}));
  REQUIRE_THROWS(x.unflatten(1, {5, -1}));
  REQUIRE_THROWS(pluss::series::empty({0}, pluss::ScalarType::Float64)
                     .unflatten(0, {0, -1}));
  REQUIRE_THROWS(x.unflatten(1, {std::numeric_limits<std::int64_t>::max(), 2}));
}

TEST_CASE("unwrap applies modular correction per selected axis",
          "[series][unwrap][final-teachings]") {
  const double pi = 3.14159265358979323846;
  auto phase = pluss::make_series({0.0, 1.0, 2.0, 4.0, -2.0, 0.0});
  const auto expected = std::vector<double>{0.0, 1.0, 2.0, 4.0,
                                            -2.0 + 2.0 * pi, 2.0 * pi};
  const auto actual = phase.unwrap().to_vec<double>();
  for (std::size_t i = 0; i < actual.size(); ++i)
    REQUIRE(actual[i] == Approx(expected[i]).margin(1e-12));
  REQUIRE(phase.unwrap(0.5).allclose(phase.unwrap(pi), 1e-12, 1e-12));

  const auto multi = pluss::make_series({0.0, 13.0, -13.0}).unwrap().to_vec<double>();
  REQUIRE(multi[1] == Approx(13.0 - 4.0 * pi).margin(1e-12));
  REQUIRE(multi[2] == Approx(-13.0 + 4.0 * pi).margin(1e-12));

  const auto matrix = pluss::make_series({{0.0, 4.0}, {0.0, -4.0}}).unwrap(pi, 0);
  REQUIRE(matrix.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.to_vec<double>()[2] == Approx(0.0));
  REQUIRE(matrix.to_vec<double>()[3] == Approx(2.0 * pi - 4.0).margin(1e-12));

  phase.tracks_slope_(true);
  phase.unwrap().sum().reverse();
  REQUIRE(phase.sensitivity().allclose(pluss::series::ones(
      {6}, pluss::ScalarType::Float64)));

  REQUIRE(pluss::series::empty({0}, pluss::ScalarType::Float64).unwrap().numel() == 0);
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).unwrap());
  REQUIRE_THROWS(pluss::make_series(1.0).unwrap());
}
