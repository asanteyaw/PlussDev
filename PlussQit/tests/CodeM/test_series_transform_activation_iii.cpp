#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <complex>
#include <cstdint>
#include <vector>

namespace {

using Catch::Approx;
using Complex = std::complex<double>;

pluss::Series spectrum(const pluss::Series& signal, bool normalized = false,
                       bool onesided = true) {
  return signal.stft(4, 2, 4, pluss::series::undefined(), true, "reflect",
                     normalized, onesided, true);
}

}  // namespace

TEST_CASE("isreal classifies each lane and preserves shape", "[series][isreal]") {
  const auto real = pluss::make_series({{1.0, -2.0}, {0.0, 4.0}}).isreal();
  REQUIRE(real.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(real.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(real.to_vec<bool>() == std::vector<bool>{true, true, true, true});

  const auto complex = pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {2.0, -0.0}, {3.0, 4.0}, {0.0, -2.0}}, {2, 2});
  REQUIRE(complex.isreal().to_vec<bool>() ==
          std::vector<bool>{true, true, false, false});
}

TEST_CASE("item extracts exactly one lane into native or tagged scalars", "[series][item]") {
  REQUIRE(pluss::make_series(7).item<std::int64_t>() == 7);
  REQUIRE(pluss::make_series({3.5}).item<double>() == Approx(3.5));
  REQUIRE(pluss::make_series(true).item<bool>());
  REQUIRE(pluss::make_series(7).item().dtype() == pluss::ScalarType::Int64);
  REQUIRE(pluss::make_series(std::vector<Complex>{{2.0, -3.0}}, {}).item<Complex>() ==
          Complex{2.0, -3.0});
  REQUIRE_THROWS(pluss::make_series({1, 2}).item<std::int64_t>());
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 2.0}}, {}).item());
}

TEST_CASE("layer_norm enforces trailing shape and differentiates all affine inputs",
          "[series][layer_norm]") {
  auto x = pluss::make_series({{1.0, 2.0, 3.0}, {2.0, 4.0, 6.0}});
  auto weight = pluss::make_series({1.0, 2.0, 0.5});
  auto bias = pluss::make_series({0.0, 1.0, -1.0});
  const auto y = x.layer_norm({3}, weight, bias);
  REQUIRE(y.sizes() == x.sizes());
  REQUIRE(y.to_vec<double>()[0] == Approx(-1.2247356859).margin(1e-9));
  REQUIRE(y.to_vec<double>()[1] == Approx(1.0).margin(1e-12));
  REQUIRE(y.to_vec<double>()[2] == Approx(-0.3876302003).margin(1e-9));

  x.tracks_slope_(true);
  weight.tracks_slope_(true);
  bias.tracks_slope_(true);
  x.layer_norm({3}, weight, bias).sum().reverse();
  REQUIRE(x.has_sensitivity());
  REQUIRE(weight.has_sensitivity());
  REQUIRE(bias.has_sensitivity());
  REQUIRE(bias.sensitivity().to_vec<double>() == std::vector<double>{2.0, 2.0, 2.0});

  REQUIRE_THROWS(x.layer_norm({2}, pluss::make_series({1.0, 1.0}),
                              pluss::make_series({0.0, 0.0})));
  REQUIRE_THROWS(x.layer_norm({3}, pluss::make_series({1.0}), bias));
  REQUIRE_THROWS(pluss::make_series({{1, 2}, {3, 4}}).layer_norm(
      {2}, pluss::make_series({1, 1}), pluss::make_series({0, 0})));
}

TEST_CASE("mish and selu preserve shape, promote integers, and carry reverse rules",
          "[series][mish][selu]") {
  const auto x = pluss::make_series({{-2.0, 0.0}, {1.0, 3.0}});
  REQUIRE(x.mish().sizes() == x.sizes());
  REQUIRE(x.selu().sizes() == x.sizes());
  REQUIRE(pluss::make_series({-2, 0, 2}).mish().scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(pluss::make_series({-2, 0, 2}).selu().scalar_type() == pluss::ScalarType::Float32);

  auto mish_x = pluss::make_series({-1.0, 0.0, 1.0});
  mish_x.tracks_slope_(true);
  mish_x.mish().sum().reverse();
  REQUIRE(mish_x.sensitivity().to_vec<double>()[1] == Approx(0.6).margin(1e-12));

  auto selu_x = pluss::make_series({-1.0, 0.0, 1.0});
  selu_x.tracks_slope_(true);
  selu_x.selu().sum().reverse();
  REQUIRE(selu_x.sensitivity().to_vec<double>()[2] == Approx(1.0507009873554805));

  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).mish());
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).selu());
}

TEST_CASE("rrelu separates deterministic evaluation from sampled training",
          "[series][rrelu]") {
  const auto x = pluss::make_series({-2.0, -1.0, 0.0, 2.0});
  const auto positive_slopes = x.rrelu(0.2, 0.4, false).to_vec<double>();
  REQUIRE(positive_slopes[0] == Approx(-0.6));
  REQUIRE(positive_slopes[1] == Approx(-0.3));
  REQUIRE(positive_slopes[2] == Approx(0.0));
  REQUIRE(positive_slopes[3] == Approx(2.0));
  const auto negative_slopes = x.rrelu(-0.4, -0.2, false).to_vec<double>();
  REQUIRE(negative_slopes[0] == Approx(0.6));
  REQUIRE(negative_slopes[1] == Approx(0.3));
  REQUIRE(negative_slopes[2] == Approx(0.0));
  REQUIRE(negative_slopes[3] == Approx(2.0));

  pluss::rng_seed(20260928);
  auto tracked = x.clone();
  tracked.tracks_slope_(true);
  const auto sampled = tracked.rrelu(0.2, 0.4, true);
  sampled.sum().reverse();
  const auto values = sampled.to_vec<double>();
  const auto gradients = tracked.sensitivity().to_vec<double>();
  REQUIRE(gradients[0] == Approx(values[0] / -2.0));
  REQUIRE(gradients[1] == Approx(values[1] / -1.0));
  REQUIRE(gradients[2] >= 0.2);
  REQUIRE(gradients[2] <= 0.4);
  REQUIRE(gradients[3] == Approx(1.0));

  REQUIRE_THROWS(x.rrelu(0.5, 0.1, true));
  REQUIRE_THROWS(pluss::make_series({-2, 1}).rrelu());
}

TEST_CASE("istft reconstructs vectors and batches and pads requested tails",
          "[series][istft]") {
  const auto signal = pluss::make_series({1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
  REQUIRE(spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                                 false, true, 8).allclose(signal, 1e-10, 1e-10));
  REQUIRE(spectrum(signal, true).istft(4, 2, 4, pluss::series::undefined(), true,
                                       true, true, 8).allclose(signal, 1e-10, 1e-10));

  const auto window = pluss::make_series({0.5, 1.0, 1.0, 0.5});
  const auto windowed = signal.stft(4, 2, 4, window, true, "reflect", false, true, true);
  REQUIRE(windowed.istft(4, 2, 4, window, true, false, true, 8)
              .allclose(signal, 1e-10, 1e-10));

  const auto uncentered = signal.stft(4, 2, 4, pluss::series::undefined(), false,
                                      "reflect", false, true, true);
  REQUIRE(uncentered.istft(4, 2, 4, pluss::series::undefined(), false,
                           false, true, 8).allclose(signal, 1e-10, 1e-10));

  const auto batch = pluss::make_series({{1.0, 2.0, 3.0, 4.0, 3.0, 2.0, 1.0, 0.0},
                                         {0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0}});
  REQUIRE(spectrum(batch).istft(4, 2, 4, pluss::series::undefined(), true,
                                false, true, 8).allclose(batch, 1e-10, 1e-10));

  const auto padded = spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                                              false, true, 11);
  REQUIRE(padded.sizes() == std::vector<std::int64_t>{11});
  REQUIRE(padded.select(0, 10).item<double>() == Approx(0.0));

  REQUIRE_THROWS(signal.istft(4));
  REQUIRE_THROWS(spectrum(signal).istft(4, 2, 4, pluss::make_series({1.0, 1.0})));
  REQUIRE_THROWS(spectrum(signal).istft(4, 2, 4, pluss::series::zeros(
      {4}, pluss::ScalarType::Float64)));
  REQUIRE_THROWS(spectrum(signal).istft(4, 2, 4, pluss::series::undefined(), true,
                                        false, true, 0));
}
