#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::Platform;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;

struct ParityTolerance final {
  double atol{1e-12};
  double rtol{1e-10};
};

void require_close_vec(const Series& got,
                       const std::vector<double>& expected,
                       const ParityTolerance tol,
                       const std::string& label) {
  const auto gv = got.host<double>();
  REQUIRE(gv.size() == expected.size());
  for (std::size_t i = 0; i < gv.size(); ++i) {
    const double scale = std::abs(expected[i]);
    const double limit = tol.atol + tol.rtol * scale;
    INFO(label << " idx=" << i << " expected=" << expected[i] << " got=" << gv[i] << " tol=" << limit);
    REQUIRE(std::abs(gv[i] - expected[i]) <= limit);
  }
}

} // namespace

TEST_CASE("cpu autodiff parity harness matches reference indexing gradients", "[autodiff][parity][cpu]") {
  const ParityTolerance tol{};

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    const Series idx = make_series(std::vector<std::int64_t>{3, 1, 0}, {3});
    x.index({idx}).sum().reverse();
    require_close_vec(x.sensitivity(), {1.0, 1.0, 0.0, 1.0}, tol, "index_single");
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    const Series i0 = make_series(std::vector<std::int64_t>{0, 1, 1, 0}, {4});
    const Series i1 = make_series(std::vector<std::int64_t>{2, 0, 2, 2}, {4});
    x.index({i0, i1}).sum().reverse();
    require_close_vec(x.sensitivity(), {0.0, 0.0, 2.0, 1.0, 0.0, 1.0}, tol, "index_mixed");
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    const Series mask = make_series(std::vector<std::int64_t>{1, 0, 1, 1}, {4}).to(ScalarType::Bool);
    x.index({mask}).sum().reverse();
    require_close_vec(x.sensitivity(), {1.0, 0.0, 1.0, 1.0}, tol, "index_mask");
  }

  {
    Series x = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    const Series idx = make_series(std::vector<std::int64_t>{3, 1, 3, 0}, {4});
    x.take(idx).sum().reverse();
    require_close_vec(x.sensitivity(), {1.0, 1.0, 0.0, 2.0}, tol, "take");
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    const Series idx = make_series(std::vector<std::int64_t>{0, 1, 1, 0}, {2, 2});
    x.gather(1, idx).sum().reverse();
    require_close_vec(x.sensitivity(), {1.0, 1.0, 1.0, 1.0}, tol, "gather");
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(ScalarType::Float64);
    REQUIRE(x.platform() == Platform::CPU);
    x.tracks_slope_(true);
    x.select(0, 1).sum().reverse();
    require_close_vec(x.sensitivity(), {0.0, 0.0, 1.0, 1.0}, tol, "select");
  }
}

TEST_CASE("cpu autodiff parity harness matches reference reduction gradients", "[autodiff][parity][cpu]") {
  Series x = make_series(std::vector<double>{0.2, -0.3, 0.4}, {3}).to(ScalarType::Float64);
  REQUIRE(x.platform() == Platform::CPU);
  x.tracks_slope_(true);

  const Series loss = x.square().mean().add(x.exp().add(1.0).log().sum());
  loss.reverse();

  const auto xv = x.host<double>();
  std::vector<double> expected;
  expected.reserve(xv.size());
  for (const double v : xv) {
    const double grad_mean_square = (2.0 / 3.0) * v;
    const double grad_softplus = 1.0 / (1.0 + std::exp(-v));
    expected.push_back(grad_mean_square + grad_softplus);
  }
  require_close_vec(x.sensitivity(), expected, ParityTolerance{1e-11, 1e-10}, "scalar_objective");
}

TEST_CASE("cpu autodiff parity harness matches reference simple training lane gradients", "[autodiff][parity][cpu]") {
  const Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}).to(ScalarType::Float64);
  const Series y_true = make_series(std::vector<double>{2.0, 4.0, 6.0}, {3}).to(ScalarType::Float64);
  REQUIRE(x.platform() == Platform::CPU);
  REQUIRE(y_true.platform() == Platform::CPU);

  Series w = make_series(0.5).to(ScalarType::Float64);
  Series b = make_series(0.1).to(ScalarType::Float64);
  w.tracks_slope_(true);
  b.tracks_slope_(true);

  const Series pred = x.mul(w).add(b);
  const Series residual = pred.sub(y_true);
  const Series loss = residual.square().mean();
  loss.reverse();

  // Reference scalar lane:
  // grad_w = 2/N * sum((w*x+b-y) * x), grad_b = 2/N * sum(w*x+b-y), N=3
  REQUIRE(w.sensitivity().item<double>() == Approx(-13.6).margin(1e-12));
  REQUIRE(b.sensitivity().item<double>() == Approx(-5.8).margin(1e-12));
}
