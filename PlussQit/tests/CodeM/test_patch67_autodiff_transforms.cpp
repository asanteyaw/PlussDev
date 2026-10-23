#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pluss/codem/codem.h>
#include <pluss/codem/series_factory.h>
#include <pluss/subsp/math/diff.h>

#include <array>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::make_series;
using pluss::ScalarType;
using pluss::Series;
using pluss::codem::ExtractSpec;
using pluss::subsp::math::diff::curvature_audit;
using pluss::subsp::math::diff::curvature_audit_report;
using pluss::subsp::math::diff::DerivativeAuditOptions;
using pluss::subsp::math::diff::DerivativePolicy;
using pluss::subsp::math::diff::DisconnectedSensitivity;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::gradient_audit;
using pluss::subsp::math::diff::gradient_audit_report;
using pluss::subsp::math::diff::hessian;
using pluss::subsp::math::diff::HessianOptions;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::hvp_lanes;
using pluss::subsp::math::diff::HvpOptions;
using pluss::subsp::math::diff::jacfwd;
using pluss::subsp::math::diff::jacobian;
using pluss::subsp::math::diff::JacobianMode;
using pluss::subsp::math::diff::JacobianOptions;
using pluss::subsp::math::diff::jacrev;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::jvp_lanes;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_diff;
using pluss::subsp::math::diff::ReverseDebugScope;
using pluss::subsp::math::diff::ReversePolicy;
using pluss::subsp::math::diff::slope_source;
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::trace_recording_enabled;
using pluss::subsp::math::diff::TraceRecordingScope;
using pluss::subsp::math::diff::value_and_grad;
using pluss::subsp::math::diff::vhp;
using pluss::subsp::math::diff::vhp_lanes;
using pluss::subsp::math::diff::vjp;
using pluss::subsp::math::diff::vjp_lanes;

Series make_f64(std::vector<double> data) {
  return make_series(std::move(data), {3}).to(ScalarType::Float64);
}

std::vector<double> analytic_slope(const std::vector<double> &x) {
  std::vector<double> out;
  out.reserve(x.size());
  for (double v : x) {
    out.push_back(std::cos(v) + 2.0 * v);
  }
  return out;
}

template <class X> auto vector_objective(const X &x) {
  using pluss::subsp::math::diff::sin;
  using pluss::subsp::math::diff::square;
  return sin(x) + square(x);
}

template <class X> auto scalar_objective(const X &x) {
  using pluss::subsp::math::diff::exp;
  using pluss::subsp::math::diff::log;
  using pluss::subsp::math::diff::mean;
  using pluss::subsp::math::diff::square;
  using pluss::subsp::math::diff::sum;
  return mean(square(x)) + sum(log(exp(x) + 1.0));
}

Series scalar_objective(const Series &x) {
  return x.square().mean().add(x.exp().add(1.0).log().sum());
}

double scalar_value(const Series &x) {
  return scalar_objective(x).item<double>();
}

} // namespace

TEST_CASE("autodiff reverse preserves scalar identity reductions",
          "[autodiff][reverse]") {
  Series x = make_series(1.25).to(ScalarType::Float64);
  x.tracks_slope_(true);

  Series loss = x.square().sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(2.5));
}

TEST_CASE("autodiff reverse keeps the shape of a one-element operand broadcast against a vector",
          "[autodiff][reverse][broadcast]") {
  // A shape-[1] operand multiplied or added against a shape-[5] operand must receive a
  // shape-[1] sensitivity (libtorch: a.grad.shape == [1]); only a true scalar stays rank 0.
  const Series t = make_series(std::vector<double>{0, 1, 2, 3, 4}, {5}).to(ScalarType::Float64);

  Series one = make_series(std::vector<double>{0.5}, {1}).to(ScalarType::Float64);
  one.tracks_slope_(true);
  (one * t).sum().reverse();
  REQUIRE(one.sensitivity().sizes().size() == 1);
  REQUIRE(one.sensitivity().sizes()[0] == 1);
  REQUIRE(one.sensitivity().item<double>() == Approx(10.0));

  Series added = make_series(std::vector<double>{0.5}, {1}).to(ScalarType::Float64);
  added.tracks_slope_(true);
  (added + t).sum().reverse();
  REQUIRE(added.sensitivity().sizes().size() == 1);
  REQUIRE(added.sensitivity().item<double>() == Approx(5.0));

  Series scalar = make_series(0.5).to(ScalarType::Float64);
  scalar.tracks_slope_(true);
  (scalar * t).sum().reverse();
  REQUIRE(scalar.sensitivity().sizes().empty());
  REQUIRE(scalar.sensitivity().item<double>() == Approx(10.0));
}

TEST_CASE("autodiff reverse propagates select sensitivities to parent",
          "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.select(0, 1).sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g.size() == 4);
  REQUIRE(g[0] == Approx(0.0));
  REQUIRE(g[1] == Approx(0.0));
  REQUIRE(g[2] == Approx(1.0));
  REQUIRE(g[3] == Approx(1.0));
}

TEST_CASE("autodiff reverse propagates multi-index index_put sensitivities",
          "[autodiff][index_put]") {
  const Series x0 = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2})
                        .to(ScalarType::Float64);
  const Series i0 =
      make_series(std::vector<std::int64_t>{0, 1}, {2}).to(ScalarType::Int64);
  const Series i1 =
      make_series(std::vector<std::int64_t>{1, 0}, {2}).to(ScalarType::Int64);

  {
    Series x = x0.clone();
    x.tracks_slope_(true);
    Series src = make_series(std::vector<double>{10.0, 20.0}, {2})
                     .to(ScalarType::Float64);
    src.tracks_slope_(true);

    const Series y = x.index_put({i0, i1}, src, /*accumulate=*/false);
    const Series loss = y.sum();
    loss.reverse();

    REQUIRE(x.has_sensitivity());
    REQUIRE(src.has_sensitivity());
    const auto gx = x.sensitivity().to_vec<double>();
    const auto gs = src.sensitivity().to_vec<double>();
    REQUIRE(gx.size() == 4);
    REQUIRE(gs.size() == 2);
    REQUIRE(gx[0] == Approx(1.0));
    REQUIRE(gx[1] == Approx(0.0));
    REQUIRE(gx[2] == Approx(0.0));
    REQUIRE(gx[3] == Approx(1.0));
    REQUIRE(gs[0] == Approx(1.0));
    REQUIRE(gs[1] == Approx(1.0));
  }

  {
    Series x = x0.clone();
    x.tracks_slope_(true);
    Series src = make_series(std::vector<double>{10.0, 20.0}, {2})
                     .to(ScalarType::Float64);
    src.tracks_slope_(true);

    const Series y = x.index_put({i0, i1}, src, /*accumulate=*/true);
    const Series loss = y.sum();
    loss.reverse();

    REQUIRE(x.has_sensitivity());
    REQUIRE(src.has_sensitivity());
    const auto gx = x.sensitivity().to_vec<double>();
    const auto gs = src.sensitivity().to_vec<double>();
    REQUIRE(gx.size() == 4);
    REQUIRE(gs.size() == 2);
    for (double g : gx)
      REQUIRE(g == Approx(1.0));
    REQUIRE(gs[0] == Approx(1.0));
    REQUIRE(gs[1] == Approx(1.0));
  }
}

TEST_CASE("autodiff reverse propagates copy_ on view slices",
          "[autodiff][copy_]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  Series src =
      make_series(std::vector<double>{10.0, 20.0}, {2}).to(ScalarType::Float64);
  src.tracks_slope_(true);

  Series v = x.slice(0, 1, 3, 1);
  v.copy_(src);
  const Series loss = x.sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(src.has_sensitivity());
  const auto gx = x.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gx.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gx[0] == Approx(1.0));
  REQUIRE(gx[1] == Approx(0.0));
  REQUIRE(gx[2] == Approx(0.0));
  REQUIRE(gx[3] == Approx(1.0));
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE("autodiff reverse propagates traverse_put_ through copy_ path",
          "[autodiff][traverse]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  Series src =
      make_series(std::vector<double>{10.0, 20.0}, {2}).to(ScalarType::Float64);
  src.tracks_slope_(true);

  const std::array<ExtractSpec, 1> specs{{ExtractSpec{1, 3}}};
  x.traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec>(specs), src);

  const Series loss = x.sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(src.has_sensitivity());
  const auto gx = x.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gx.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gx[0] == Approx(1.0));
  REQUIRE(gx[1] == Approx(0.0));
  REQUIRE(gx[2] == Approx(0.0));
  REQUIRE(gx[3] == Approx(1.0));
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE("autodiff reverse preserves reshape source leaves",
          "[autodiff][reshape]") {
  Series src =
      make_series(std::vector<double>{10.0, 20.0}, {2}).to(ScalarType::Float64);
  src.tracks_slope_(true);

  const Series shaped = src.reshape({2});
  const Series loss = shaped.sum();
  loss.reverse();

  REQUIRE(src.has_sensitivity());
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gs.size() == 2);
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE(
    "autodiff strict dependency stays connected for staged inplace write APIs",
    "[autodiff][inplace][strict]") {
  DerivativePolicy strict{};
  strict.strict_dependency = true;
  strict.warn_on_disconnected = false;

  const Series init =
      make_series(std::vector<double>{0.3, -0.1, 0.2, -0.5, 0.4, 0.7}, {2, 3})
          .to(ScalarType::Float64);

  SECTION("copy_ through select views on untracked staged base") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged = pluss::series::zeros_like(x);
          staged.select(1, 0).copy_(x.select(1, 0).mul(2.0));
          staged.select(1, 1).copy_(x.select(1, 1).square());
          staged.select(1, 2).copy_(x.select(1, 2).add(1.0));
          return Trace(staged.sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }

  SECTION("traverse_put_ on untracked staged base") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged = pluss::series::zeros_like(x);
          const std::array<ExtractSpec, 2> specs{
              ExtractSpec{pluss::Slicing::SliceAll()}, ExtractSpec{1, 3}};
          staged.traverse_put_(pluss::msharp::RangeView::Of<ExtractSpec>(specs),
                               x.slice(1, 1, 3, 1));
          return Trace(staged.sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }

  SECTION("extract_put_ on untracked staged base") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged = pluss::series::zeros_like(x);
          staged.extract_put_(
              {pluss::Slicing::SliceAll(), pluss::Slicing::SliceRange(1, 3, 1)},
              x.slice(1, 1, 3, 1));
          return Trace(staged.sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }

  SECTION("index_put_ single-index path stays connected beyond 1D-specialized "
          "case") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged =
              pluss::series::zeros({6}, x.scalar_type()).to(x.platform());
          const Series idx = make_series(std::vector<std::int64_t>{1, 4}, {2});
          const Series src = x.reshape({6}).slice(0, 0, 2, 1);
          staged.index_put_({idx}, src, false);
          return Trace(staged.sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }

  SECTION("index_put_ multi-index path stays connected on staged base") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged = pluss::series::zeros_like(x);
          const Series i0 = make_series(std::vector<std::int64_t>{0, 1}, {2})
                                .to(ScalarType::Int64);
          const Series i1 = make_series(std::vector<std::int64_t>{1, 2}, {2})
                                .to(ScalarType::Int64);
          const Series src = x.select(1, 0).reshape({2});
          staged.index_put_({i0, i1}, src, false);
          return Trace(staged.sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }

  SECTION("repeated copy_ writes on staged base remain connected") {
    const auto [value, grads] = value_and_grad(
        [](const std::vector<Trace> &xs) -> Trace {
          const Series x = xs[0].value();
          Series staged = pluss::series::zeros_like(x);
          for (std::int64_t t = 0; t < x.size(1); ++t) {
            staged.select(1, t).copy_(
                x.select(1, t).add(static_cast<double>(t + 1)));
          }
          return Trace(staged.square().sum());
        },
        std::vector<Series>{init}, strict);
    REQUIRE(value.defined());
    REQUIRE(value.numel() == 1);
    REQUIRE(grads.size() == 1);
    REQUIRE(grads[0].defined());
    REQUIRE(grads[0].abs().sum().item<double>() > 0.0);
  }
}

TEST_CASE(
    "autodiff reverse propagates take sensitivities with duplicate indices",
    "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series idx = make_series(std::vector<std::int64_t>{3, 1, 3, 0}, {4});
  const Series loss = x.take(idx).sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g.size() == 4);
  REQUIRE(g[0] == Approx(1.0));
  REQUIRE(g[1] == Approx(1.0));
  REQUIRE(g[2] == Approx(0.0));
  REQUIRE(g[3] == Approx(2.0));
}

TEST_CASE(
    "autodiff reverse distributes amax and amin ties across reduced lanes",
    "[autodiff][reverse][reduction]") {
  Series x =
      make_series(std::vector<double>{1.0, 5.0, 5.0, 2.0, -1.0, 2.0}, {2, 3})
          .to(ScalarType::Float64);
  x.tracks_slope_(true);
  x.amax(1, false).sum().reverse();
  {
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g == std::vector<double>{0.0, 0.5, 0.5, 0.5, 0.0, 0.5});
  }

  x.clear();
  x.amin(0, false).sum().reverse();
  {
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g == std::vector<double>{1.0, 0.0, 0.0, 0.0, 1.0, 1.0});
  }
}

TEST_CASE("autodiff reverse distributes full amax and amin ties across all "
          "global winners",
          "[autodiff][reverse][reduction]") {
  Series x =
      make_series(std::vector<double>{7.0, 0.0, 0.0, 7.0, 7.0, 1.0}, {2, 3})
          .to(ScalarType::Float64);
  x.tracks_slope_(true);
  x.amax().reverse();
  {
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 6);
    REQUIRE(g[0] == Approx(1.0 / 3.0));
    REQUIRE(g[1] == Approx(0.0));
    REQUIRE(g[2] == Approx(0.0));
    REQUIRE(g[3] == Approx(1.0 / 3.0));
    REQUIRE(g[4] == Approx(1.0 / 3.0));
    REQUIRE(g[5] == Approx(0.0));
  }

  x = make_series(std::vector<double>{-3.0, 0.0, 0.0, -3.0, -3.0, 1.0}, {2, 3})
          .to(ScalarType::Float64);
  x.tracks_slope_(true);
  x.amin().reverse();
  {
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 6);
    REQUIRE(g[0] == Approx(1.0 / 3.0));
    REQUIRE(g[1] == Approx(0.0));
    REQUIRE(g[2] == Approx(0.0));
    REQUIRE(g[3] == Approx(1.0 / 3.0));
    REQUIRE(g[4] == Approx(1.0 / 3.0));
    REQUIRE(g[5] == Approx(0.0));
  }
}

TEST_CASE("autodiff reverse propagates cumsum sensitivities with reverse scan",
          "[autodiff][reverse][scan]") {
  Series x =
      make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3})
          .to(ScalarType::Float64);
  x.tracks_slope_(true);
  x.cumsum(1).sum().reverse();

  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g == std::vector<double>{3.0, 2.0, 1.0, 3.0, 2.0, 1.0});
}

TEST_CASE("autodiff reverse seed shape mismatch fails loudly",
          "[autodiff][diagnostics]") {
  Series x = make_f64({0.3, -0.2, 0.5});
  x.tracks_slope_(true);
  const Series output = x.square();
  const Series bad_seed = make_series(1.0).to(ScalarType::Float64);
  REQUIRE_THROWS_WITH(output.reverse(bad_seed),
                      Catch::Matchers::ContainsSubstring("seed shape"));
}

TEST_CASE("autodiff reverse debug catches non-finite sensitivity propagation",
          "[autodiff][diagnostics]") {
  Series x = make_series(0.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.log();
  ReverseDebugScope debug_scope{};
  REQUIRE_THROWS_WITH(loss.reverse(), Catch::Matchers::ContainsSubstring(
                                          "non-finite sensitivity"));
}

TEST_CASE("autodiff reverse reports operator context on saved-value mutation",
          "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.mul(x).sum();
  x.storage().bump_version();
  REQUIRE_THROWS_WITH(loss.reverse(),
                      Catch::Matchers::ContainsSubstring("[op=mul,"));
  REQUIRE_THROWS_WITH(loss.reverse(),
                      Catch::Matchers::ContainsSubstring("captured_at="));
}

TEST_CASE("autodiff reverse reports gather operator in saved-value mutation "
          "diagnostics",
          "[autodiff][diagnostics]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series idx = make_series(std::vector<std::int64_t>{0, 1, 1, 0}, {2, 2});
  const Series loss = x.gather(1, idx).sum();
  x.storage().bump_version();
  REQUIRE_THROWS_WITH(loss.reverse(),
                      Catch::Matchers::ContainsSubstring("[op=gather,"));
}

TEST_CASE("autodiff reverse reports captured-output mutation diagnostics",
          "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.square().sum();
  loss.storage().bump_version();
  REQUIRE_THROWS_WITH(loss.reverse(),
                      Catch::Matchers::ContainsSubstring(
                          "captured output Series was modified"));
  REQUIRE_THROWS_WITH(loss.reverse(),
                      Catch::Matchers::ContainsSubstring("[op="));
}

TEST_CASE("autodiff reverse one-shot keep_graph=false fails on reuse",
          "[autodiff][diagnostics]") {
  Series x = make_series(2.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.square().sum();

  ReversePolicy one_shot{};
  one_shot.keep_graph = false;
  Trace(loss).reverse(one_shot);

  x.clear();
  REQUIRE_THROWS_WITH(Trace(loss).reverse(), Catch::Matchers::ContainsSubstring(
                                                 "one-shot reverse pass"));
}

TEST_CASE("autodiff tracked leaf in-place arithmetic stays connected",
          "[autodiff][inplace][strict]") {
  Series x = make_series(3.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  x.add_(1.0);
  const Series loss = x.square().sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(8.0).margin(1e-12));
}

TEST_CASE(
    "autodiff index propagates sensitivities for single integer index tensor",
    "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series idx = make_series(std::vector<std::int64_t>{3, 1, 0}, {3});
  const Series loss = x.index({idx}).sum();
  loss.reverse();
  REQUIRE(x.has_sensitivity());
  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g.size() == 4);
  REQUIRE(g[0] == Approx(1.0));
  REQUIRE(g[1] == Approx(1.0));
  REQUIRE(g[2] == Approx(0.0));
  REQUIRE(g[3] == Approx(1.0));
}

TEST_CASE("autodiff index propagates sensitivities for mixed integer advanced "
          "indexing on CPU",
          "[autodiff][reverse]") {
  Series x =
      make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3})
          .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series i0 = make_series(std::vector<std::int64_t>{0, 1, 1, 0}, {4});
  const Series i1 = make_series(std::vector<std::int64_t>{2, 0, 2, 2}, {4});
  const Series loss = x.index({i0, i1}).sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g.size() == 6);
  REQUIRE(g[0] == Approx(0.0));
  REQUIRE(g[1] == Approx(0.0));
  REQUIRE(g[2] == Approx(2.0));
  REQUIRE(g[3] == Approx(1.0));
  REQUIRE(g[4] == Approx(0.0));
  REQUIRE(g[5] == Approx(1.0));
}

TEST_CASE(
    "autodiff index propagates sensitivities for bool-mask indexing on CPU",
    "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                 .to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series mask = make_series(std::vector<std::int64_t>{1, 0, 1, 1}, {4})
                          .to(ScalarType::Bool);
  const Series loss = x.index({mask}).sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  const auto g = x.sensitivity().to_vec<double>();
  REQUIRE(g.size() == 4);
  REQUIRE(g[0] == Approx(1.0));
  REQUIRE(g[1] == Approx(0.0));
  REQUIRE(g[2] == Approx(1.0));
  REQUIRE(g[3] == Approx(1.0));
}

TEST_CASE("autodiff slope_source reports captured op metadata",
          "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series y = x.mul(x);
  const auto src = slope_source(y);
  REQUIRE(src.linked);
  REQUIRE(src.op == "mul");
  REQUIRE(src.parents == 2);
  REQUIRE_FALSE(src.capture_file.empty());
}

TEST_CASE("autodiff trace recording scope suppresses capture when disabled",
          "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);

  REQUIRE(trace_recording_enabled());
  {
    TraceRecordingScope pause(false);
    REQUIRE_FALSE(trace_recording_enabled());
    const Series y = x.mul(x);
    const auto y_src = slope_source(y);
    REQUIRE_FALSE(y_src.linked);
  }
  REQUIRE(trace_recording_enabled());

  const Series z = x.mul(x);
  const auto z_src = slope_source(z);
  REQUIRE(z_src.linked);
  REQUIRE(z_src.op == "mul");
}

TEST_CASE("autodiff trace recording scope nests deterministically",
          "[autodiff][diagnostics]") {
  REQUIRE(trace_recording_enabled());
  {
    TraceRecordingScope outer(false);
    REQUIRE_FALSE(trace_recording_enabled());
    {
      TraceRecordingScope inner(true);
      REQUIRE_FALSE(trace_recording_enabled());
    }
    REQUIRE_FALSE(trace_recording_enabled());
  }
  REQUIRE(trace_recording_enabled());
}

TEST_CASE("autodiff reverse policy accumulates leaf sensitivity when requested",
          "[autodiff][reverse]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Trace tracked_y(x.square().sum());

  tracked_y.reverse();
  const double g1 = x.sensitivity().item<double>();
  REQUIRE(g1 == Approx(3.0));

  ReversePolicy policy{};
  policy.keep_graph = true;
  policy.accumulate_sensitivity = true;
  tracked_y.reverse(policy);

  const double g2 = x.sensitivity().item<double>();
  REQUIRE(g2 == Approx(6.0));
}

TEST_CASE("autodiff reverse policy tracks higher-order sensitivity graph when "
          "enabled",
          "[autodiff][reverse]") {
  Series x = make_series(2.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Trace y(x * x * x);

  ReversePolicy policy{};
  policy.keep_graph = true;
  policy.track_higher_order = true;
  y.reverse(policy);
  REQUIRE(x.sensitivity().item<double>() == Approx(12.0).margin(1e-12));

  const Series first = x.sensitivity();
  x.clear();
  first.reverse();
  REQUIRE(x.sensitivity().item<double>() == Approx(12.0).margin(1e-8));
}

TEST_CASE("autodiff reverse does not build higher-order graph by default",
          "[autodiff][reverse]") {
  Series x = make_series(2.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Trace y(x * x * x);
  y.reverse();
  const Series first = x.sensitivity();
  x.clear();
  REQUIRE_THROWS(first.reverse());
  REQUIRE_FALSE(x.has_sensitivity());
}

TEST_CASE("autodiff reverse handles scalar product quotient and chain rules",
          "[autodiff][reverse]") {
  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x * x.cos();
    y.reverse();

    REQUIRE(x.has_sensitivity());
    const double expected = std::cos(1.7) - 1.7 * std::sin(1.7);
    REQUIRE(x.sensitivity().item<double>() == Approx(expected).margin(1e-12));
  }

  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.div(x.square().add(1.0));
    y.reverse();

    REQUIRE(x.has_sensitivity());
    const double expected = (1.0 - 1.7 * 1.7) / std::pow(1.0 + 1.7 * 1.7, 2.0);
    REQUIRE(x.sensitivity().item<double>() == Approx(expected).margin(1e-12));
  }

  {
    Series x = make_series(1.2).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.square().cos();
    y.reverse();

    REQUIRE(x.has_sensitivity());
    const double expected = -std::sin(1.2 * 1.2) * 2.0 * 1.2;
    REQUIRE(x.sensitivity().item<double>() == Approx(expected).margin(1e-12));
  }
}

TEST_CASE(
    "autodiff reverse distinguishes tracked inputs from output sensitivity",
    "[autodiff][reverse]") {
  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    const Series y = x * x.cos();
    REQUIRE_THROWS(y.reverse());

    REQUIRE_FALSE(x.has_sensitivity());
    REQUIRE_FALSE(y.has_sensitivity());
    REQUIRE(y.sensitivity().item<double>() == Approx(0.0).margin(1e-12));
  }

  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x * x.cos();
    y.reverse();

    REQUIRE(x.has_sensitivity());
    const double expected = std::cos(1.7) - 1.7 * std::sin(1.7);
    REQUIRE(x.sensitivity().item<double>() == Approx(expected).margin(1e-12));
    REQUIRE(y.has_sensitivity());
    REQUIRE(y.sensitivity().item<double>() == Approx(1.0).margin(1e-12));
  }
}

TEST_CASE("autodiff sensitivity examples stay verified",
          "[autodiff][reverse][docs]") {
  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);
    x.mul(3.0).sum().reverse();

    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 4);
    REQUIRE(g[0] == Approx(3.0).margin(1e-12));
    REQUIRE(g[1] == Approx(3.0).margin(1e-12));
    REQUIRE(g[2] == Approx(3.0).margin(1e-12));
    REQUIRE(g[3] == Approx(3.0).margin(1e-12));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);

    REQUIRE_FALSE(x.has_sensitivity());
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(0.0).margin(1e-12));
    REQUIRE(g[1] == Approx(0.0).margin(1e-12));
    REQUIRE(g[2] == Approx(0.0).margin(1e-12));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);

    x.mul(2.0).sum().reverse();
    {
      const auto g = x.sensitivity().to_vec<double>();
      REQUIRE(g.size() == 3);
      REQUIRE(g[0] == Approx(2.0).margin(1e-12));
      REQUIRE(g[1] == Approx(2.0).margin(1e-12));
      REQUIRE(g[2] == Approx(2.0).margin(1e-12));
    }

    x.clear();
    x.mul(5.0).sum().reverse();
    {
      const auto g = x.sensitivity().to_vec<double>();
      REQUIRE(g.size() == 3);
      REQUIRE(g[0] == Approx(5.0).margin(1e-12));
      REQUIRE(g[1] == Approx(5.0).margin(1e-12));
      REQUIRE(g[2] == Approx(5.0).margin(1e-12));
    }
  }

  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    const Series y = x * x.cos();
    REQUIRE_THROWS(y.reverse());

    REQUIRE(y.item<double>() == Approx(-0.21903564030239187).margin(1e-12));
    REQUIRE_FALSE(x.has_sensitivity());
    REQUIRE_FALSE(y.has_sensitivity());
    REQUIRE(y.sensitivity().item<double>() == Approx(0.0).margin(1e-12));
  }

  {
    Series x =
        make_series(1.7, pluss::cfg::tracks_slope()).to(ScalarType::Float64);
    const Series y = x * x.cos();
    y.reverse();

    REQUIRE(y.item<double>() == Approx(-0.21903564030239187).margin(1e-12));
    REQUIRE(x.has_sensitivity());
    REQUIRE(x.sensitivity().item<double>() ==
            Approx(std::cos(1.7) - 1.7 * std::sin(1.7)).margin(1e-12));
    REQUIRE(y.has_sensitivity());
    REQUIRE(y.sensitivity().item<double>() == Approx(1.0).margin(1e-12));
  }

  {
    Series x = make_series(1.7).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.div(x.square().add(1.0));
    y.reverse();

    REQUIRE(y.item<double>() == Approx(0.43701799485861187).margin(1e-12));
    REQUIRE(x.sensitivity().item<double>() ==
            Approx(-0.12490004692012345).margin(1e-12));
  }

  {
    Series x = make_series(1.2).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.square().cos();
    y.reverse();

    REQUIRE(y.item<double>() == Approx(0.13042370873814554).margin(1e-12));
    REQUIRE(x.sensitivity().item<double>() ==
            Approx(-2.3795000356600475).margin(1e-12));
  }

  {
    Series x = make_series(0.25).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.atanh();
    y.reverse();

    REQUIRE(y.item<double>() == Approx(0.2554128118829953).margin(1e-12));
    REQUIRE(x.sensitivity().item<double>() ==
            Approx(1.0666666666666667).margin(1e-12));
  }

  {
    Series x = make_series(0.4).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.erf();
    y.reverse();

    REQUIRE(y.item<double>() == Approx(0.42839235504666845).margin(1e-12));
    REQUIRE(x.sensitivity().item<double>() ==
            Approx(0.9615412988393078).margin(1e-11));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series objective = 2.0 * x.pos(0) + 4.0 * x.pos(1) + x.pos(2).sqrt();
    objective.reverse();

    REQUIRE(objective.item<double>() ==
            Approx(11.732050807568877).margin(1e-12));
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(2.0).margin(1e-12));
    REQUIRE(g[1] == Approx(4.0).margin(1e-12));
    REQUIRE(g[2] == Approx(0.28867513459481287).margin(1e-12));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series objective =
        x.pos(0).sin() - 1.5 * x.pos(1).cos() + x.pos(2).log();
    objective.reverse();

    REQUIRE(objective.item<double>() == Approx(2.56430352829672).margin(1e-12));
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(0.5403023058681398).margin(1e-12));
    REQUIRE(g[1] == Approx(1.3639461402385225).margin(1e-12));
    REQUIRE(g[2] == Approx(0.3333333333333333).margin(1e-12));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series objective =
        x.pos(0).sin() - 1.5 * x.pos(1).cos() * x.pos(2).log();
    objective.reverse();

    REQUIRE(objective.item<double>() ==
            Approx(1.5272470275894714).margin(1e-12));
    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(0.5403023058681398).margin(1e-12));
    REQUIRE(g[1] == Approx(1.4984479907474777).margin(1e-12));
    REQUIRE(g[2] == Approx(0.2080734182735712).margin(1e-12));
  }

  {
    Series x = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                   .to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = x.mul(x);
    y.reverse(make_series(std::vector<double>{1.0, 10.0, 100.0}, {3})
                  .to(ScalarType::Float64));

    const auto g = x.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(2.0).margin(1e-12));
    REQUIRE(g[1] == Approx(40.0).margin(1e-12));
    REQUIRE(g[2] == Approx(600.0).margin(1e-12));
  }
}

TEST_CASE("autodiff reverse attaches unary math sensitivities",
          "[autodiff][reverse]") {
  auto require_scalar_slope = [](double x_value,
                                 const std::function<Series(const Series &)> &f,
                                 double expected, double margin = 1e-12) {
    Series x = make_series(x_value).to(ScalarType::Float64);
    x.tracks_slope_(true);
    const Series y = f(x);
    y.reverse();
    REQUIRE(x.has_sensitivity());
    REQUIRE(x.sensitivity().item<double>() == Approx(expected).margin(margin));
  };

  require_scalar_slope(
      0.3, [](const Series &x) { return x.tan(); },
      1.0 / std::pow(std::cos(0.3), 2.0));
  require_scalar_slope(
      0.5, [](const Series &x) { return x.asin(); },
      1.0 / std::sqrt(1.0 - 0.5 * 0.5));
  require_scalar_slope(
      0.5, [](const Series &x) { return x.acos(); },
      -1.0 / std::sqrt(1.0 - 0.5 * 0.5));
  require_scalar_slope(
      1.5, [](const Series &x) { return x.atan(); }, 1.0 / (1.0 + 1.5 * 1.5));
  require_scalar_slope(
      0.4, [](const Series &x) { return x.sinh(); }, std::cosh(0.4));
  require_scalar_slope(
      0.4, [](const Series &x) { return x.cosh(); }, std::sinh(0.4));
  require_scalar_slope(
      0.4, [](const Series &x) { return x.log1p(); }, 1.0 / 1.4);
  require_scalar_slope(
      0.4, [](const Series &x) { return x.expm1(); }, std::exp(0.4));
  require_scalar_slope(
      2.0, [](const Series &x) { return x.reciprocal(); }, -0.25);
  require_scalar_slope(
      0.4, [](const Series &x) { return x.erf(); },
      1.1283791670955126 * std::exp(-(0.4 * 0.4)), 1e-11);
  require_scalar_slope(
      0.4, [](const Series &x) { return x.erfc(); },
      -1.1283791670955126 * std::exp(-(0.4 * 0.4)), 1e-11);
  require_scalar_slope(
      0.4, [](const Series &x) { return x.asinh(); },
      1.0 / std::sqrt(1.0 + 0.4 * 0.4), 1e-11);
  require_scalar_slope(
      1.8, [](const Series &x) { return x.acosh(); },
      1.0 / (std::sqrt(1.8 - 1.0) * std::sqrt(1.8 + 1.0)), 1e-11);
  require_scalar_slope(
      0.25, [](const Series &x) { return x.atanh(); },
      1.0 / (1.0 - 0.25 * 0.25), 1e-11);
}

TEST_CASE(
    "autodiff reverse handles mixed scalar objectives over selected entries",
    "[autodiff][reverse]") {
  Series reuse = make_series(std::vector<double>{1.0, 2.0, 3.0}, {3})
                     .to(ScalarType::Float64);
  reuse.tracks_slope_(true);

  (2.0 * reuse.pos(0) + 4.0 * reuse.pos(1) + reuse.pos(2).sqrt()).reverse();
  {
    const auto g = reuse.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(2.0).margin(1e-12));
    REQUIRE(g[1] == Approx(4.0).margin(1e-12));
    REQUIRE(g[2] == Approx(0.5 / std::sqrt(3.0)).margin(1e-12));
  }

  reuse.clear();
  (reuse.pos(0).sin() - 1.5 * reuse.pos(1).cos() + reuse.pos(2).log())
      .reverse();
  {
    const auto g = reuse.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(std::cos(1.0)).margin(1e-12));
    REQUIRE(g[1] == Approx(1.5 * std::sin(2.0)).margin(1e-12));
    REQUIRE(g[2] == Approx(1.0 / 3.0).margin(1e-12));
  }

  reuse.clear();
  (reuse.pos(0).sin() - 1.5 * reuse.pos(1).cos() * reuse.pos(2).log())
      .reverse();
  {
    const auto g = reuse.sensitivity().to_vec<double>();
    REQUIRE(g.size() == 3);
    REQUIRE(g[0] == Approx(std::cos(1.0)).margin(1e-12));
    REQUIRE(g[1] == Approx(1.5 * std::sin(2.0) * std::log(3.0)).margin(1e-12));
    REQUIRE(g[2] == Approx(-1.5 * std::cos(2.0) / 3.0).margin(1e-12));
  }
}

TEST_CASE("reverse-mode primitive families pass numerical gradient audits",
          "[autodiff][audit][reverse]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-6;
  opt.atol = 1e-6;
  opt.rtol = 1e-4;

  REQUIRE(gradient_audit(
      [](const auto &z) {
        using pluss::subsp::math::diff::acos;
        using pluss::subsp::math::diff::acosh;
        using pluss::subsp::math::diff::asin;
        using pluss::subsp::math::diff::asinh;
        using pluss::subsp::math::diff::atan;
        using pluss::subsp::math::diff::atanh;
        using pluss::subsp::math::diff::abs;
        using pluss::subsp::math::diff::cos;
        using pluss::subsp::math::diff::cosh;
        using pluss::subsp::math::diff::erf;
        using pluss::subsp::math::diff::erfc;
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::expm1;
        using pluss::subsp::math::diff::log;
        using pluss::subsp::math::diff::log1p;
        using pluss::subsp::math::diff::reciprocal;
        using pluss::subsp::math::diff::relu;
        using pluss::subsp::math::diff::rsqrt;
        using pluss::subsp::math::diff::sigmoid;
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::sinh;
        using pluss::subsp::math::diff::sqrt;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        using pluss::subsp::math::diff::tan;
        using pluss::subsp::math::diff::tanh;
        return sum(exp(z * 0.3) + log(z + 2.0) + sqrt(z + 2.0) + sin(z) +
                   cos(z) + tanh(z) + tan(z * 0.2) + asin(z * 0.2) +
                   acos(z * 0.2) + atan(z) + sinh(z) + cosh(z) +
                   log1p(z + 2.0) + expm1(z * 0.1) + reciprocal(z + 2.0) +
                   rsqrt(z + 2.0) + erf(z * 0.3) + erfc(z * 0.3) + abs(z) +
                   asinh(z) + acosh(z + 1.5) + atanh(z * 0.4) + relu(z) +
                   sigmoid(z) + square(z));
      },
      x, opt));
}

TEST_CASE("reverse-mode handles scalar objectives beyond plain sums",
          "[autodiff][audit][reverse]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-6;
  opt.atol = 1e-6;
  opt.rtol = 1e-4;

  REQUIRE(gradient_audit(
      [](const auto &z) {
        using pluss::subsp::math::diff::cos;
        using pluss::subsp::math::diff::log;
        using pluss::subsp::math::diff::mean;
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        const auto energy = sum(square(z)) + 3.0;
        return mean(z * cos(z)) * log(energy) - sum(sin(z)) / energy;
      },
      x, opt));
}

TEST_CASE("autodiff reverse policy can drop source links after pass",
          "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series y = x.square().sum();
  REQUIRE(slope_source(y).linked);

  ReversePolicy once{};
  once.keep_graph = false;
  Trace(y).reverse(once);

  REQUIRE_FALSE(slope_source(y).linked);
}

TEST_CASE("autodiff grad policy tracks higher-order graph when requested",
          "[autodiff][transforms]") {
  Series x = make_series(2.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  DerivativePolicy policy{};
  policy.track_higher_order = true;
  const Series first =
      grad([](const Trace &z) -> Trace { return z * z * z; }, x, policy);
  REQUIRE(first.item<double>() == Approx(12.0).margin(1e-12));

  x.clear();
  first.reverse();
  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(12.0).margin(1e-8));
}

TEST_CASE("autodiff default grad materializes zero for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  const Series g = grad(
      [](const Trace &) -> Trace {
        return trace(make_series(1.0).to(ScalarType::Float64), false);
      },
      x);
  REQUIRE(g.item<double>() == Approx(0.0));
}

TEST_CASE("autodiff grad can return undefined for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy policy{};
  policy.disconnected_sensitivity = DisconnectedSensitivity::Undefined;
  const Series g = grad(
      [](const Trace &) -> Trace {
        return trace(make_series(1.0).to(ScalarType::Float64), false);
      },
      x, policy);
  REQUIRE_FALSE(g.defined());
}

TEST_CASE("autodiff strict grad fails for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy strict{};
  strict.strict_dependency = true;
  REQUIRE_THROWS_WITH(
      grad(
          [](const Trace &) -> Trace {
            return trace(make_series(1.0).to(ScalarType::Float64), false);
          },
          x, strict),
      Catch::Matchers::ContainsSubstring("disconnected from input"));
}

TEST_CASE("autodiff strict vjp fails for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy strict{};
  strict.strict_dependency = true;
  auto [value, pullback] = vjp(
      [](const Trace &) -> Trace {
        return trace(make_series(3.0).to(ScalarType::Float64), false);
      },
      x, strict);
  REQUIRE(value.item<double>() == Approx(3.0));
  REQUIRE_THROWS_WITH(
      pullback(make_series(1.0).to(ScalarType::Float64)),
      Catch::Matchers::ContainsSubstring("disconnected from input"));
}

TEST_CASE("autodiff vjp can return undefined for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy policy{};
  policy.disconnected_sensitivity = DisconnectedSensitivity::Undefined;
  auto [value, pullback] = vjp(
      [](const Trace &) -> Trace {
        return trace(make_series(3.0).to(ScalarType::Float64), false);
      },
      x, policy);
  REQUIRE(value.item<double>() == Approx(3.0));
  const Series g = pullback(make_series(1.0).to(ScalarType::Float64));
  REQUIRE_FALSE(g.defined());
}

TEST_CASE("autodiff vjp pullback is reusable by default",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  auto [value, pullback] =
      vjp([](const Trace &z) -> Trace { return z * z * z; }, x);
  REQUIRE(value.item<double>() == Approx(8.0).margin(1e-12));

  const Series seed = make_series(1.0).to(ScalarType::Float64);
  const Series g1 = pullback(seed);
  const Series g2 = pullback(seed);
  REQUIRE(g1.item<double>() == Approx(12.0).margin(1e-12));
  REQUIRE(g2.item<double>() == Approx(12.0).margin(1e-12));
}

TEST_CASE("autodiff vjp keep_graph=false computes pullback",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy policy{};
  policy.keep_graph = false;
  auto [value, pullback] =
      vjp([](const Trace &z) -> Trace { return z * z * z; }, x, policy);
  REQUIRE(value.item<double>() == Approx(8.0).margin(1e-12));

  const Series seed = make_series(1.0).to(ScalarType::Float64);
  const Series g1 = pullback(seed);
  REQUIRE(g1.item<double>() == Approx(12.0).margin(1e-12));
}

TEST_CASE("autodiff vjp keep_graph=false pullback is one-shot",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  DerivativePolicy policy{};
  policy.keep_graph = false;
  auto [value, pullback] =
      vjp([](const Trace &z) -> Trace { return z * z * z; }, x, policy);
  REQUIRE(value.item<double>() == Approx(8.0).margin(1e-12));

  const Series seed = make_series(1.0).to(ScalarType::Float64);
  const Series g1 = pullback(seed);
  REQUIRE(g1.item<double>() == Approx(12.0).margin(1e-12));
  REQUIRE_THROWS_WITH(pullback(seed),
                      Catch::Matchers::ContainsSubstring("one-shot"));
}

TEST_CASE("autodiff grad policy can accumulate input sensitivities",
          "[autodiff][transforms]") {
  Series x = make_series(2.0).to(ScalarType::Float64);
  x.tracks_slope_(true);

  DerivativePolicy policy{};
  policy.accumulate_input_sensitivity = true;
  const Series g1 =
      grad([](const Trace &z) -> Trace { return z * z; }, x, policy);
  const Series g2 =
      grad([](const Trace &z) -> Trace { return z * z; }, x, policy);

  REQUIRE(g1.item<double>() == Approx(4.0).margin(1e-12));
  REQUIRE(g2.item<double>() == Approx(8.0).margin(1e-12));
  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(8.0).margin(1e-12));
}

TEST_CASE("autodiff seeded grad supports non-scalar objectives",
          "[autodiff][transforms]") {
  const Series x = make_series(std::vector<double>{0.2, -0.3, 0.4}, {3})
                       .to(ScalarType::Float64);
  const Series cot = make_series(std::vector<double>{1.0, -2.0, 0.5}, {3})
                         .to(ScalarType::Float64);

  const Series g =
      grad([](const Trace &z) -> Trace { return vector_objective(z); }, x, cot);

  const auto xv = x.to_vec<double>();
  const auto gv = g.to_vec<double>();
  const auto cotv = cot.to_vec<double>();
  REQUIRE(gv.size() == xv.size());
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = cotv[i] * (std::cos(xv[i]) + 2.0 * xv[i]);
    REQUIRE(gv[i] == Approx(expected).margin(1e-10));
  }
}

TEST_CASE("autodiff seeded value_and_grad returns value and pullback",
          "[autodiff][transforms]") {
  const Series x = make_series(std::vector<double>{0.2, -0.3, 0.4}, {3})
                       .to(ScalarType::Float64);
  const Series cot = make_series(std::vector<double>{1.0, -2.0, 0.5}, {3})
                         .to(ScalarType::Float64);

  const auto [value, g] = value_and_grad(
      [](const Trace &z) -> Trace { return vector_objective(z); }, x, cot);

  const auto vv = value.to_vec<double>();
  const auto xv = x.to_vec<double>();
  REQUIRE(vv.size() == xv.size());
  for (std::size_t i = 0; i < xv.size(); ++i) {
    REQUIRE(vv[i] == Approx(std::sin(xv[i]) + xv[i] * xv[i]).margin(1e-10));
  }

  const auto gv = g.to_vec<double>();
  REQUIRE(gv.size() == xv.size());
  const auto cotv = cot.to_vec<double>();
  for (std::size_t i = 0; i < xv.size(); ++i) {
    const double expected = cotv[i] * (std::cos(xv[i]) + 2.0 * xv[i]);
    REQUIRE(gv[i] == Approx(expected).margin(1e-10));
  }
}

TEST_CASE("autodiff seeded vjp matches closure pullback semantics",
          "[autodiff][transforms]") {
  const Series x = make_series(std::vector<double>{0.2, -0.3, 0.4}, {3})
                       .to(ScalarType::Float64);
  const Series cot = make_series(std::vector<double>{1.0, -2.0, 0.5}, {3})
                         .to(ScalarType::Float64);

  const auto [value_direct, grad_direct] =
      vjp([](const Trace &z) -> Trace { return vector_objective(z); }, x, cot);

  const auto [value_closure, pullback] =
      vjp([](const Trace &z) -> Trace { return vector_objective(z); }, x);
  const Series grad_closure = pullback(cot);

  const auto vd = value_direct.to_vec<double>();
  const auto vc = value_closure.to_vec<double>();
  REQUIRE(vd.size() == vc.size());
  for (std::size_t i = 0; i < vd.size(); ++i) {
    REQUIRE(vd[i] == Approx(vc[i]).margin(1e-12));
  }

  const auto gd = grad_direct.to_vec<double>();
  const auto gc = grad_closure.to_vec<double>();
  REQUIRE(gd.size() == gc.size());
  for (std::size_t i = 0; i < gd.size(); ++i) {
    REQUIRE(gd[i] == Approx(gc[i]).margin(1e-12));
  }
}

TEST_CASE("autodiff strict seeded grad fails for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(std::vector<double>{0.2, -0.3, 0.4}, {3})
                       .to(ScalarType::Float64);
  const Series cot = make_series(std::vector<double>{1.0, -2.0, 0.5}, {3})
                         .to(ScalarType::Float64);
  DerivativePolicy strict{};
  strict.strict_dependency = true;

  REQUIRE_THROWS_WITH(
      grad(
          [](const Trace &) -> Trace {
            return trace(make_series(std::vector<double>{3.0, 3.0, 3.0}, {3})
                             .to(ScalarType::Float64),
                         false);
          },
          x, cot, strict),
      Catch::Matchers::ContainsSubstring("disconnected from input"));
}

TEST_CASE("autodiff strict hvp fails for disconnected objective",
          "[autodiff][transforms]") {
  const Series x = make_series(2.0).to(ScalarType::Float64);
  const Series v = make_series(1.0).to(ScalarType::Float64);
  DerivativePolicy strict{};
  strict.strict_dependency = true;
  REQUIRE_THROWS_WITH(
      hvp(
          [](const Trace &) -> Trace {
            return trace(make_series(1.0).to(ScalarType::Float64), false);
          },
          x, v, HvpOptions{}, strict),
      Catch::Matchers::ContainsSubstring("disconnected from input"));
}

TEST_CASE("autodiff tracked batched matmul reverse matches finite difference",
          "[autodiff][reverse]") {
  std::vector<double> a_data{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  const std::vector<double> b_data{1.5, -0.5, 0.2, 0.8, -1.0, 0.3, 0.7, -0.4};
  const Series b = make_series(b_data, {2, 2, 2}).to(ScalarType::Float64);

  Series a = make_series(a_data, {2, 2, 2}).to(ScalarType::Float64);
  a.tracks_slope_(true);
  const Series loss = a.matmul(b).sum();
  loss.reverse();

  const auto g = a.sensitivity().to_vec<double>();
  REQUIRE(g.size() == a_data.size());
  const double eps = 1e-6;
  for (std::size_t i = 0; i < a_data.size(); ++i) {
    auto ap = a_data;
    auto am = a_data;
    ap[i] += eps;
    am[i] -= eps;
    const Series a_plus = make_series(ap, {2, 2, 2}).to(ScalarType::Float64);
    const Series a_minus = make_series(am, {2, 2, 2}).to(ScalarType::Float64);
    const double fp = a_plus.matmul(b).sum().item<double>();
    const double fm = a_minus.matmul(b).sum().item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(g[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE(
    "autodiff tracked broadcasted batched matmul reverse reduces correctly",
    "[autodiff][reverse]") {
  std::vector<double> a_data{1.0, 2.0, 3.0, 4.0};
  const std::vector<double> b_data{1.5, -0.5, 0.2, 0.8, -1.0, 0.3, 0.7, -0.4};
  const Series b = make_series(b_data, {2, 2, 2}).to(ScalarType::Float64);

  Series a = make_series(a_data, {1, 2, 2}).to(ScalarType::Float64);
  a.tracks_slope_(true);
  const Series loss = a.matmul(b).sum();
  loss.reverse();

  const auto g = a.sensitivity().to_vec<double>();
  REQUIRE(g.size() == a_data.size());
  const double eps = 1e-6;
  for (std::size_t i = 0; i < a_data.size(); ++i) {
    auto ap = a_data;
    auto am = a_data;
    ap[i] += eps;
    am[i] -= eps;
    const Series a_plus = make_series(ap, {1, 2, 2}).to(ScalarType::Float64);
    const Series a_minus = make_series(am, {1, 2, 2}).to(ScalarType::Float64);
    const double fp = a_plus.matmul(b).sum().item<double>();
    const double fm = a_minus.matmul(b).sum().item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(g[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE(
    "autodiff tracked vector-matrix matmul reverse matches finite difference",
    "[autodiff][reverse]") {
  std::vector<double> a_data{1.0, 2.0, 3.0, 4.0};
  const std::vector<double> b_data{1.0, 0.0, 0.0, 1.0, 2.0, 1.0, -1.0, 0.5};
  const Series b = make_series(b_data, {4, 2}).to(ScalarType::Float64);

  Series a = make_series(a_data, {4}).to(ScalarType::Float64);
  a.tracks_slope_(true);
  const Series loss = a.matmul(b).sum();
  loss.reverse();

  const auto g = a.sensitivity().to_vec<double>();
  REQUIRE(g.size() == a_data.size());
  const double eps = 1e-6;
  for (std::size_t i = 0; i < a_data.size(); ++i) {
    auto ap = a_data;
    auto am = a_data;
    ap[i] += eps;
    am[i] -= eps;
    const Series a_plus = make_series(ap, {4}).to(ScalarType::Float64);
    const Series a_minus = make_series(am, {4}).to(ScalarType::Float64);
    const double fp = a_plus.matmul(b).sum().item<double>();
    const double fm = a_minus.matmul(b).sum().item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(g[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE(
    "autodiff tracked matrix-vector matmul reverse matches finite difference",
    "[autodiff][reverse]") {
  std::vector<double> a_data{2.0, 1.0, 0.0, -1.0, 3.0, 2.0};
  const std::vector<double> b_data{3.0, 1.0, 2.0};
  const Series a = make_series(a_data, {2, 3}).to(ScalarType::Float64);

  Series b = make_series(b_data, {3}).to(ScalarType::Float64);
  b.tracks_slope_(true);
  const Series loss = a.matmul(b).sum();
  loss.reverse();

  const auto g = b.sensitivity().to_vec<double>();
  REQUIRE(g.size() == b_data.size());
  const double eps = 1e-6;
  for (std::size_t i = 0; i < b_data.size(); ++i) {
    auto bp = b_data;
    auto bm = b_data;
    bp[i] += eps;
    bm[i] -= eps;
    const Series b_plus = make_series(bp, {3}).to(ScalarType::Float64);
    const Series b_minus = make_series(bm, {3}).to(ScalarType::Float64);
    const double fp = a.matmul(b_plus).sum().item<double>();
    const double fm = a.matmul(b_minus).sum().item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(g[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE(
    "autodiff tracked vector-vector matmul reverse matches finite difference",
    "[autodiff][reverse]") {
  std::vector<double> a_data{1.0, -2.0, 0.5};
  std::vector<double> b_data{0.25, 1.5, -3.0};

  Series a = make_series(a_data, {3}).to(ScalarType::Float64);
  Series b = make_series(b_data, {3}).to(ScalarType::Float64);
  a.tracks_slope_(true);
  b.tracks_slope_(true);
  const Series loss = a.matmul(b);
  loss.reverse();

  const auto ga = a.sensitivity().to_vec<double>();
  const auto gb = b.sensitivity().to_vec<double>();
  REQUIRE(ga.size() == a_data.size());
  REQUIRE(gb.size() == b_data.size());

  const double eps = 1e-6;
  for (std::size_t i = 0; i < a_data.size(); ++i) {
    auto ap = a_data;
    auto am = a_data;
    ap[i] += eps;
    am[i] -= eps;
    const Series a_plus = make_series(ap, {3}).to(ScalarType::Float64);
    const Series a_minus = make_series(am, {3}).to(ScalarType::Float64);
    const double fp = a_plus.matmul(b).item<double>();
    const double fm = a_minus.matmul(b).item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(ga[i] == Approx(fd).margin(1e-5));
  }
  for (std::size_t i = 0; i < b_data.size(); ++i) {
    auto bp = b_data;
    auto bm = b_data;
    bp[i] += eps;
    bm[i] -= eps;
    const Series b_plus = make_series(bp, {3}).to(ScalarType::Float64);
    const Series b_minus = make_series(bm, {3}).to(ScalarType::Float64);
    const double fp = a.matmul(b_plus).item<double>();
    const double fm = a.matmul(b_minus).item<double>();
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(gb[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE("autodiff scatter_add propagates sensitivities to base and source",
          "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                    .to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4}, {4})
                   .to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 2, 2, 3}, {4});
  base.tracks_slope_(true);
  src.tracks_slope_(true);
  REQUIRE(base.tracks_slope());
  REQUIRE(src.tracks_slope());

  const Series out = base.scatter_add(0, idx, src);
  const auto src_info = slope_source(out);
  REQUIRE(src_info.linked);
  REQUIRE(src_info.op == "scatter_add");
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 4);
  for (double v : gb)
    REQUIRE(v == Approx(1.0));
  for (double v : gs)
    REQUIRE(v == Approx(1.0));
}

TEST_CASE("autodiff scatter propagates sensitivities to base and source",
          "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                    .to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{5.0, 6.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter(0, idx, src);
  const auto src_info = slope_source(out);
  REQUIRE(src_info.linked);
  REQUIRE(src_info.op == "scatter");

  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gb[0] == Approx(1.0));
  REQUIRE(gb[1] == Approx(0.0));
  REQUIRE(gb[2] == Approx(1.0));
  REQUIRE(gb[3] == Approx(0.0));
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE(
    "autodiff scatter_add scalar source sensitivity equals contribution count",
    "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0}, {4})
                    .to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{2.0}, {1}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 1, 1, 3}, {4});
  base.tracks_slope_(true);
  src.tracks_slope_(true);
  REQUIRE(base.tracks_slope());
  REQUIRE(src.tracks_slope());
  const Series out = base.scatter_add(0, idx, src);
  const auto src_info = slope_source(out);
  REQUIRE(src_info.linked);
  REQUIRE(src_info.op == "scatter_add");
  const Series loss = out.sum();
  loss.reverse();

  REQUIRE(base.has_sensitivity());
  REQUIRE(src.has_sensitivity());
  const auto gb = base.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  for (double v : gb)
    REQUIRE(v == Approx(1.0));
  REQUIRE(src.sensitivity().item<double>() == Approx(4.0));
}

TEST_CASE("autodiff scatter_reduce(sum) propagates sensitivities",
          "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                    .to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{0.5, -1.0, 2.0, 1.5}, {4})
                   .to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 2, 2, 3}, {4});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out =
      base.scatter_reduce(0, idx, src, "sum", /*include_self=*/true);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 4);
  for (double v : gb)
    REQUIRE(v == Approx(1.0));
  for (double v : gs)
    REQUIRE(v == Approx(1.0));
}

TEST_CASE("autodiff scatter_reduce(sum, include_self=false) zeros base "
          "sensitivity at scattered indices",
          "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                    .to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{7.0, 8.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out =
      base.scatter_reduce(0, idx, src, "sum", /*include_self=*/false);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gb[0] == Approx(1.0));
  REQUIRE(gb[1] == Approx(0.0));
  REQUIRE(gb[2] == Approx(1.0));
  REQUIRE(gb[3] == Approx(0.0));
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE("autodiff scatter_reduce(prod) handles receiver and source zeros",
          "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4})
                    .to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{0.0, 5.0, 0.0}, {3})
                   .to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 1, 3}, {3});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  base.scatter_reduce(0, idx, src, "prod", /*include_self=*/true)
      .sum()
      .reverse();

  REQUIRE(base.sensitivity().to_vec<double>() ==
          std::vector<double>{1.0, 0.0, 1.0, 0.0});
  REQUIRE(src.sensitivity().to_vec<double>() ==
          std::vector<double>{10.0, 0.0, 4.0});
}

TEST_CASE("autodiff index backward accepts an empty index into an empty input",
          "[autodiff][reverse][index]") {
  Series input = Series::empty({0, 3}, ScalarType::Float64);
  input.tracks_slope_(true);
  const Series index = Series::empty({0}, ScalarType::Int64);

  input.index({index}).sum().reverse();

  REQUIRE(input.sensitivity().sizes() == input.sizes());
  REQUIRE(input.sensitivity().numel() == 0);
}

TEST_CASE(
    "autodiff scatter_reduce(amax) distributes ties across base and source",
    "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{2.0, 5.0, 5.0, 1.0}, {4})
                    .to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{5.0, 4.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out =
      base.scatter_reduce(0, idx, src, "amax", /*include_self=*/true);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gb[0] == Approx(1.0));
  REQUIRE(gb[1] == Approx(0.5));
  REQUIRE(gb[2] == Approx(1.0));
  REQUIRE(gb[3] == Approx(0.0));
  REQUIRE(gs[0] == Approx(0.5));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE("autodiff elementwise extrema follow torch tie routing",
          "[autodiff][reverse][parity]") {
  for (const bool maximum : {false, true}) {
    Series lhs = make_series(std::vector<double>{1.0, 2.0, 4.0}, {3})
                     .to(ScalarType::Float64);
    Series rhs = make_series(std::vector<double>{2.0, 2.0, 1.0}, {3})
                     .to(ScalarType::Float64);
    lhs.tracks_slope_(true);
    rhs.tracks_slope_(true);

    (maximum ? lhs.maximum(rhs) : lhs.minimum(rhs)).sum().reverse();

    const std::vector<double> expected_lhs =
        maximum ? std::vector<double>{0.0, 0.5, 1.0}
                : std::vector<double>{1.0, 0.5, 0.0};
    const std::vector<double> expected_rhs =
        maximum ? std::vector<double>{1.0, 0.5, 0.0}
                : std::vector<double>{0.0, 0.5, 1.0};
    REQUIRE(lhs.sensitivity().to_vec<double>() == expected_lhs);
    REQUIRE(rhs.sensitivity().to_vec<double>() == expected_rhs);
  }
}

TEST_CASE("autodiff scatter_reduce(amin, include_self=false) propagates only "
          "through source",
          "[autodiff][reverse]") {
  Series base =
      make_series(std::vector<double>{1.0, 2.0}, {2}).to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{-3.0, 0.5}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 1}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out =
      base.scatter_reduce(0, idx, src, "amin", /*include_self=*/false);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 2);
  REQUIRE(gs.size() == 2);
  REQUIRE(gb[0] == Approx(0.0));
  REQUIRE(gb[1] == Approx(0.0));
  REQUIRE(gs[0] == Approx(1.0));
  REQUIRE(gs[1] == Approx(1.0));
}

TEST_CASE("scatter_reduce(include_self=false) keeps untouched self lanes",
          "[autodiff][reverse]") {
  const Series base =
      make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4})
          .to(ScalarType::Float64);
  const Series src =
      make_series(std::vector<double>{1.0, 3.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 1}, {2});

  const Series out =
      base.scatter_reduce(0, idx, src, "sum", /*include_self=*/false);
  const auto v = out.to_vec<double>();
  REQUIRE(v.size() == 4);
  REQUIRE(v[0] == Approx(10.0));
  REQUIRE(v[1] == Approx(4.0));
  REQUIRE(v[2] == Approx(30.0));
  REQUIRE(v[3] == Approx(40.0));
}

TEST_CASE(
    "autodiff scatter_reduce(mean) gradients match count-normalized rules",
    "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4})
                    .to(ScalarType::Float64);
  Series src =
      make_series(std::vector<double>{1.0, 3.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 1}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out =
      base.scatter_reduce(0, idx, src, "mean", /*include_self=*/true);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 2);
  REQUIRE(gb[0] == Approx(1.0));
  REQUIRE(gb[1] == Approx(1.0 / 3.0));
  REQUIRE(gb[2] == Approx(1.0));
  REQUIRE(gb[3] == Approx(1.0));
  REQUIRE(gs[0] == Approx(1.0 / 3.0));
  REQUIRE(gs[1] == Approx(1.0 / 3.0));
}

TEST_CASE("value_and_grad matches analytic and finite-difference gradients",
          "[autodiff][grad]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto [value, g] = value_and_grad(
      [](const Trace &z) -> Trace { return scalar_objective(z); }, x);

  REQUIRE(value.numel() == 1);

  const auto grad_v = g.to_vec<double>();
  const auto x_v = x.to_vec<double>();
  const double eps = 1e-6;

  for (std::size_t i = 0; i < grad_v.size(); ++i) {
    auto xp = x_v;
    auto xm = x_v;
    xp[i] += eps;
    xm[i] -= eps;
    const double fp = scalar_value(make_f64(xp));
    const double fm = scalar_value(make_f64(xm));
    const double fd = (fp - fm) / (2.0 * eps);
    REQUIRE(grad_v[i] == Approx(fd).margin(1e-5));
  }
}

TEST_CASE("vjp matches cotangent pullback analytically", "[autodiff][vjp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series cotangent = make_f64({1.0, -0.5, 0.25});
  const auto [value, pullback] =
      vjp([](const Trace &z) -> Trace { return vector_objective(z); }, x);
  const Series pulled = pullback(cotangent);

  REQUIRE(value.sizes()[0] == 3);

  const auto expected_slope = analytic_slope(x.to_vec<double>());
  const auto c = cotangent.to_vec<double>();
  const auto got = pulled.to_vec<double>();
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == Approx(expected_slope[i] * c[i]).margin(1e-12));
  }
}

TEST_CASE("vjp_lanes computes multiple pullbacks with consistent value",
          "[autodiff][vjp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const std::vector<Series> cotangent_lanes{
      make_f64({1.0, -0.5, 0.25}),
      make_f64({-0.2, 0.4, 0.1}),
  };

  const auto [value, lanes] =
      vjp_lanes([](const Trace &z) -> Trace { return vector_objective(z); }, x,
                cotangent_lanes);
  REQUIRE(value.sizes()[0] == 3);
  REQUIRE(lanes.size() == cotangent_lanes.size());

  const auto slope = analytic_slope(x.to_vec<double>());
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto c = cotangent_lanes[lane].to_vec<double>();
    const auto g = lanes[lane].to_vec<double>();
    REQUIRE(g.size() == c.size());
    for (std::size_t i = 0; i < g.size(); ++i) {
      REQUIRE(g[i] == Approx(slope[i] * c[i]).margin(1e-12));
    }
  }
}

TEST_CASE("vjp_lanes auto-retains graph for multi-lane pullbacks",
          "[autodiff][vjp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const std::vector<Series> cotangent_lanes{
      make_f64({1.0, -0.5, 0.25}),
      make_f64({-0.2, 0.4, 0.1}),
  };

  DerivativePolicy policy{};
  policy.keep_graph = false;
  const auto [value, lanes] =
      vjp_lanes([](const Trace &z) -> Trace { return vector_objective(z); }, x,
                cotangent_lanes, policy);
  REQUIRE(value.sizes()[0] == 3);
  REQUIRE(lanes.size() == cotangent_lanes.size());

  const auto slope = analytic_slope(x.to_vec<double>());
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto c = cotangent_lanes[lane].to_vec<double>();
    const auto g = lanes[lane].to_vec<double>();
    REQUIRE(g.size() == c.size());
    for (std::size_t i = 0; i < g.size(); ++i) {
      REQUIRE(g[i] == Approx(slope[i] * c[i]).margin(1e-12));
    }
  }
}

TEST_CASE("jvp matches exact vector-output directional derivative",
          "[autodiff][jvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, directional] =
      jvp([](const DualTrace &z) -> DualTrace { return vector_objective(z); },
          x, tangent);

  REQUIRE(value.sizes()[0] == 3);

  const auto slope = analytic_slope(x.to_vec<double>());
  const auto t = tangent.to_vec<double>();
  const auto got = directional.to_vec<double>();
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == Approx(slope[i] * t[i]).margin(1e-12));
  }
}

TEST_CASE("jvp_lanes computes multiple directional lanes with consistent value",
          "[autodiff][jvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const std::vector<Series> tangent_lanes{
      make_f64({0.1, 0.05, -0.2}),
      make_f64({-0.3, 0.2, 0.4}),
  };

  const auto [value, lanes] = jvp_lanes(
      [](const DualTrace &z) -> DualTrace { return vector_objective(z); }, x,
      tangent_lanes);
  REQUIRE(value.sizes()[0] == 3);
  REQUIRE(lanes.size() == tangent_lanes.size());

  const auto slope = analytic_slope(x.to_vec<double>());
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto t = tangent_lanes[lane].to_vec<double>();
    const auto g = lanes[lane].to_vec<double>();
    REQUIRE(g.size() == t.size());
    for (std::size_t i = 0; i < g.size(); ++i) {
      REQUIRE(g[i] == Approx(slope[i] * t[i]).margin(1e-12));
    }
  }
}

TEST_CASE("expanded unary derivative coverage remains analytic-accurate",
          "[autodiff][coverage]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, directional] = jvp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::acos;
        using pluss::subsp::math::diff::asin;
        using pluss::subsp::math::diff::atan;
        using pluss::subsp::math::diff::cosh;
        using pluss::subsp::math::diff::erf;
        using pluss::subsp::math::diff::erfc;
        using pluss::subsp::math::diff::expm1;
        using pluss::subsp::math::diff::log1p;
        using pluss::subsp::math::diff::reciprocal;
        using pluss::subsp::math::diff::sinh;
        return asin(z * 0.2) + acos(z * 0.2) + atan(z) + sinh(z) + cosh(z) +
               log1p(z + 2.0) + expm1(z * 0.1) + reciprocal(z + 2.0) +
               erf(z * 0.3) + erfc(z * 0.3);
      },
      x, tangent);

  REQUIRE(value.sizes()[0] == 3);
  const auto xv = x.to_vec<double>();
  const auto tv = tangent.to_vec<double>();
  const auto dv = directional.to_vec<double>();
  constexpr double kTwoOverSqrtPi = 1.12837916709551257390;
  for (std::size_t i = 0; i < dv.size(); ++i) {
    const double s = xv[i];
    const double slope =
        0.2 / std::sqrt(1.0 - (0.2 * s) * (0.2 * s)) +
        (-0.2 / std::sqrt(1.0 - (0.2 * s) * (0.2 * s))) + 1.0 / (1.0 + s * s) +
        std::cosh(s) + std::sinh(s) + 1.0 / (s + 3.0) +
        0.1 * std::exp(0.1 * s) + (-1.0 / std::pow(s + 2.0, 2.0)) +
        0.3 * kTwoOverSqrtPi * std::exp(-(0.3 * s) * (0.3 * s)) +
        (-0.3 * kTwoOverSqrtPi * std::exp(-(0.3 * s) * (0.3 * s)));
    REQUIRE(dv[i] == Approx(slope * tv[i]).margin(1e-10));
  }
}

TEST_CASE("extended unary and activation derivative coverage remains "
          "analytic-accurate",
          "[autodiff][coverage]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, directional] = jvp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::acosh;
        using pluss::subsp::math::diff::asinh;
        using pluss::subsp::math::diff::atanh;
        using pluss::subsp::math::diff::gelu;
        using pluss::subsp::math::diff::leaky_relu;
        using pluss::subsp::math::diff::silu;
        using pluss::subsp::math::diff::tan;
        return tan(z) + asinh(z) + acosh(z + 1.5) + atanh(z * 0.4) +
               leaky_relu(z, 0.2) + silu(z) + gelu(z, false) + gelu(z, true);
      },
      x, tangent);

  REQUIRE(value.sizes()[0] == 3);
  const auto xv = x.to_vec<double>();
  const auto tv = tangent.to_vec<double>();
  const auto dv = directional.to_vec<double>();
  constexpr double kInvSqrt2 = 0.70710678118654752440;
  constexpr double kInvSqrt2Pi = 0.39894228040143267794;
  constexpr double kC = 0.79788456080286535588; // sqrt(2/pi)
  constexpr double kK = 0.044715;
  for (std::size_t i = 0; i < dv.size(); ++i) {
    const double s = xv[i];
    const double tan_slope = 1.0 / (std::cos(s) * std::cos(s));
    const double asinh_slope = 1.0 / std::sqrt(1.0 + s * s);
    const double a = s + 1.5;
    const double acosh_slope = 1.0 / (std::sqrt(a - 1.0) * std::sqrt(a + 1.0));
    const double atanh_slope = 0.4 / (1.0 - (0.4 * s) * (0.4 * s));
    const double leaky_slope = s > 0.0 ? 1.0 : 0.2;
    const double sig = 1.0 / (1.0 + std::exp(-s));
    const double silu_slope = sig * (1.0 + s * (1.0 - sig));
    const double gelu_exact_slope = 0.5 * (1.0 + std::erf(s * kInvSqrt2)) +
                                    s * kInvSqrt2Pi * std::exp(-0.5 * s * s);

    const double u = kC * (s + kK * s * s * s);
    const double t = std::tanh(u);
    const double du = kC * (1.0 + 3.0 * kK * s * s);
    const double gelu_approx_slope =
        0.5 * (1.0 + t) + 0.5 * s * (1.0 - t * t) * du;

    const double slope = tan_slope + asinh_slope + acosh_slope + atanh_slope +
                         leaky_slope + silu_slope + gelu_exact_slope +
                         gelu_approx_slope;
    REQUIRE(dv[i] == Approx(slope * tv[i]).margin(2e-9));
  }
}

TEST_CASE(
    "softmax and log_softmax directional derivatives are analytic-accurate",
    "[autodiff][coverage]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});

  const auto [softmax_value, softmax_dir] = jvp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::softmax;
        return softmax(z, 0);
      },
      x, tangent);
  const auto [logsoftmax_value, logsoftmax_dir] = jvp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::log_softmax;
        return log_softmax(z, 0);
      },
      x, tangent);

  const auto y = softmax_value.to_vec<double>();
  const auto v = tangent.to_vec<double>();
  const auto dy = softmax_dir.to_vec<double>();
  const auto dlog = logsoftmax_dir.to_vec<double>();

  double dot = 0.0;
  double sum_v = 0.0;
  for (std::size_t i = 0; i < y.size(); ++i) {
    dot += y[i] * v[i];
    sum_v += v[i];
  }
  for (std::size_t i = 0; i < y.size(); ++i) {
    REQUIRE(dy[i] == Approx(y[i] * (v[i] - dot)).margin(1e-12));
    REQUIRE(dlog[i] == Approx(v[i] - y[i] * sum_v).margin(1e-12));
  }

  const auto lsm = logsoftmax_value.to_vec<double>();
  for (std::size_t i = 0; i < y.size(); ++i) {
    REQUIRE(std::exp(lsm[i]) == Approx(y[i]).margin(1e-12));
  }
}

TEST_CASE("loss helper transforms match analytic first-order derivatives",
          "[autodiff][loss]") {
  const Series pred = make_f64({0.3, -0.2, 0.5});
  const Series target = make_f64({0.1, -0.4, 0.9});

  const auto [mse_value, mse_grad] = value_and_grad(
      [&](const Trace &p) -> Trace {
        using pluss::subsp::math::diff::squared_error_loss;
        return squared_error_loss(p, target);
      },
      pred);

  const auto [l1_value, l1_grad] = value_and_grad(
      [&](const Trace &p) -> Trace {
        using pluss::subsp::math::diff::absolute_error_loss;
        return absolute_error_loss(p, target);
      },
      pred);

  const auto [huber_value, huber_grad] = value_and_grad(
      [&](const Trace &p) -> Trace {
        using pluss::subsp::math::diff::huber_error_loss;
        return huber_error_loss(p, target, 0.25);
      },
      pred);

  const auto [logit_value, logit_grad] = value_and_grad(
      [&](const Trace &p) -> Trace {
        using pluss::subsp::math::diff::logistic_binary_loss;
        return logistic_binary_loss(p, target);
      },
      pred);

  const auto pv = pred.to_vec<double>();
  const auto tv = target.to_vec<double>();
  const auto mse_g = mse_grad.to_vec<double>();
  const auto l1_g = l1_grad.to_vec<double>();
  const auto huber_g = huber_grad.to_vec<double>();
  const auto logit_g = logit_grad.to_vec<double>();

  const double n = static_cast<double>(pv.size());
  double mse_expected = 0.0;
  double l1_expected = 0.0;
  double huber_expected = 0.0;
  double logit_expected = 0.0;
  for (std::size_t i = 0; i < pv.size(); ++i) {
    const double err = pv[i] - tv[i];
    const double abs_err = std::abs(err);
    mse_expected += err * err;
    l1_expected += abs_err;
    if (abs_err <= 0.25) {
      huber_expected += 0.5 * err * err;
      REQUIRE(huber_g[i] == Approx(err / n).margin(1e-12));
    } else {
      huber_expected += 0.25 * (abs_err - 0.125);
      REQUIRE(huber_g[i] ==
              Approx((err > 0.0 ? 0.25 : -0.25) / n).margin(1e-12));
    }
    REQUIRE(mse_g[i] == Approx((2.0 * err) / n).margin(1e-12));
    REQUIRE(l1_g[i] == Approx((err > 0.0 ? 1.0 : -1.0) / n).margin(1e-12));

    const double sig = 1.0 / (1.0 + std::exp(-pv[i]));
    const double term = (1.0 - tv[i]) * std::log1p(std::exp(pv[i])) +
                        tv[i] * std::log1p(std::exp(-pv[i]));
    logit_expected += term;
    REQUIRE(logit_g[i] == Approx((sig - tv[i]) / n).margin(1e-12));
  }

  REQUIRE(mse_value.item<double>() == Approx(mse_expected / n).margin(1e-12));
  REQUIRE(l1_value.item<double>() == Approx(l1_expected / n).margin(1e-12));
  REQUIRE(huber_value.item<double>() ==
          Approx(huber_expected / n).margin(1e-12));
  REQUIRE(logit_value.item<double>() ==
          Approx(logit_expected / n).margin(1e-12));
}

TEST_CASE("linearize reuses a stable pushforward closure",
          "[autodiff][linearize]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto [value, pushforward] = linearize(
      [](const DualTrace &z) -> DualTrace { return vector_objective(z); }, x);

  REQUIRE(value.sizes()[0] == 3);

  const Series tangent_a = make_f64({0.1, 0.05, -0.2});
  const Series tangent_b = make_f64({-0.3, 0.2, 0.15});
  const Series out_a = pushforward(tangent_a);
  const Series out_b = pushforward(tangent_b);

  const auto slope = analytic_slope(x.to_vec<double>());
  const auto ta = tangent_a.to_vec<double>();
  const auto tb = tangent_b.to_vec<double>();
  const auto ga = out_a.to_vec<double>();
  const auto gb = out_b.to_vec<double>();

  for (std::size_t i = 0; i < ga.size(); ++i) {
    REQUIRE(ga[i] == Approx(slope[i] * ta[i]).margin(1e-12));
    REQUIRE(gb[i] == Approx(slope[i] * tb[i]).margin(1e-12));
  }
}

TEST_CASE("jacrev and jacfwd agree with analytic Jacobian",
          "[autodiff][jacobian]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto objective = [](const auto &z) { return vector_objective(z); };

  const auto [value_r, jr] = jacrev(objective, x);
  const auto [value_f, jf] = jacfwd(objective, x);

  REQUIRE(value_r.sizes()[0] == 3);
  REQUIRE(value_f.sizes()[0] == 3);
  REQUIRE(jr.sizes()[0] == 3);
  REQUIRE(jr.sizes()[1] == 3);
  REQUIRE(jf.sizes()[0] == 3);
  REQUIRE(jf.sizes()[1] == 3);

  const auto slope = analytic_slope(x.to_vec<double>());
  const auto rev = jr.to_vec<double>();
  const auto fwd = jf.to_vec<double>();

  for (std::size_t row = 0; row < slope.size(); ++row) {
    for (std::size_t col = 0; col < slope.size(); ++col) {
      const double expected = row == col ? slope[row] : 0.0;
      const std::size_t offset = row * slope.size() + col;
      REQUIRE(rev[offset] == Approx(expected).margin(1e-12));
      REQUIRE(fwd[offset] == Approx(expected).margin(1e-12));
    }
  }
}

TEST_CASE("jacobian wrapper dispatches reverse and forward modes",
          "[autodiff][jacobian]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto objective = [](const auto &z) { return vector_objective(z); };

  JacobianOptions rev_opt{};
  rev_opt.mode = JacobianMode::Reverse;
  const auto [_, j_rev] = jacobian(objective, x, rev_opt);

  JacobianOptions fwd_opt{};
  fwd_opt.mode = JacobianMode::Forward;
  const auto [__, j_fwd] = jacobian(objective, x, fwd_opt);

  const auto rev = j_rev.to_vec<double>();
  const auto fwd = j_fwd.to_vec<double>();
  REQUIRE(rev.size() == fwd.size());
  for (std::size_t i = 0; i < rev.size(); ++i) {
    REQUIRE(rev[i] == Approx(fwd[i]).margin(1e-12));
  }
}

TEST_CASE("jacobian vectorized hint preserves Jacobian semantics",
          "[autodiff][jacobian]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto objective = [](const auto &z) { return vector_objective(z); };

  JacobianOptions base_rev{};
  base_rev.mode = JacobianMode::Reverse;
  const auto [_, j_base_rev] = jacobian(objective, x, base_rev);

  JacobianOptions vec_rev{};
  vec_rev.mode = JacobianMode::Reverse;
  vec_rev.vectorized = true;
  const auto [__, j_vec_rev] = jacobian(objective, x, vec_rev);

  const auto base_rev_vals = j_base_rev.to_vec<double>();
  const auto vec_rev_vals = j_vec_rev.to_vec<double>();
  REQUIRE(base_rev_vals.size() == vec_rev_vals.size());
  for (std::size_t i = 0; i < base_rev_vals.size(); ++i) {
    REQUIRE(vec_rev_vals[i] == Approx(base_rev_vals[i]).margin(1e-12));
  }

  JacobianOptions base_fwd{};
  base_fwd.mode = JacobianMode::Forward;
  const auto [___, j_base_fwd] = jacobian(objective, x, base_fwd);

  JacobianOptions vec_fwd{};
  vec_fwd.mode = JacobianMode::Forward;
  vec_fwd.vectorized = true;
  const auto [____, j_vec_fwd] = jacobian(objective, x, vec_fwd);

  const auto base_fwd_vals = j_base_fwd.to_vec<double>();
  const auto vec_fwd_vals = j_vec_fwd.to_vec<double>();
  REQUIRE(base_fwd_vals.size() == vec_fwd_vals.size());
  for (std::size_t i = 0; i < base_fwd_vals.size(); ++i) {
    REQUIRE(vec_fwd_vals[i] == Approx(base_fwd_vals[i]).margin(1e-12));
  }
}

TEST_CASE("jacobian forward strict dependency is enforced",
          "[autodiff][jacobian]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  JacobianOptions opt{};
  opt.mode = JacobianMode::Forward;
  opt.policy.strict_dependency = true;

  REQUIRE_THROWS_WITH(
      jacobian(
          [](const auto &z) {
            return pluss::subsp::math::diff::constant_like(
                z, make_series(std::vector<double>{1.0, 1.0, 1.0}, {3})
                       .to(ScalarType::Float64));
          },
          x, opt),
      Catch::Matchers::ContainsSubstring("disconnected from input"));
}

TEST_CASE("hvp matches analytic Hessian-vector product for scalar objective",
          "[autodiff][hvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, hv] = hvp(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, tangent, HvpOptions{1e-6});

  REQUIRE(value.numel() == 1);

  const auto xv = x.to_vec<double>();
  const auto tv = tangent.to_vec<double>();
  const auto hvv = hv.to_vec<double>();
  for (std::size_t i = 0; i < hvv.size(); ++i) {
    const double expected = (2.0 - std::sin(xv[i])) * tv[i];
    REQUIRE(hvv[i] == Approx(expected).margin(1e-5));
  }
}

TEST_CASE(
    "hvp_lanes computes multiple Hessian-vector lanes with consistent value",
    "[autodiff][hvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const std::vector<Series> tangent_lanes{
      make_f64({0.1, 0.05, -0.2}),
      make_f64({-0.25, 0.0, 0.4}),
  };

  const auto [value, lanes] = hvp_lanes(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, tangent_lanes, HvpOptions{1e-6});

  REQUIRE(value.numel() == 1);
  REQUIRE(lanes.size() == tangent_lanes.size());

  const auto xv = x.to_vec<double>();
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto tv = tangent_lanes[lane].to_vec<double>();
    const auto hvv = lanes[lane].to_vec<double>();
    REQUIRE(hvv.size() == tv.size());
    for (std::size_t i = 0; i < hvv.size(); ++i) {
      const double expected = (2.0 - std::sin(xv[i])) * tv[i];
      REQUIRE(hvv[i] == Approx(expected).margin(1e-5));
    }
  }
}

TEST_CASE("vhp matches hvp for scalar objectives", "[autodiff][vhp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series direction = make_f64({0.1, 0.05, -0.2});

  const auto [value_h, hv] = hvp(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction, HvpOptions{1e-6});

  const auto [value_v, vh] = vhp(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction, HvpOptions{1e-6});

  REQUIRE(value_v.item<double>() ==
          Approx(value_h.item<double>()).margin(1e-12));
  const auto hvv = hv.to_vec<double>();
  const auto vvv = vh.to_vec<double>();
  REQUIRE(hvv.size() == vvv.size());
  for (std::size_t i = 0; i < hvv.size(); ++i) {
    REQUIRE(vvv[i] == Approx(hvv[i]).margin(1e-8));
  }
}

TEST_CASE("vhp_lanes computes multiple lanes consistent with hvp_lanes",
          "[autodiff][vhp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const std::vector<Series> direction_lanes{
      make_f64({0.1, 0.05, -0.2}),
      make_f64({-0.25, 0.0, 0.4}),
  };

  const auto [value_h, hv_lanes_out] = hvp_lanes(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction_lanes, HvpOptions{1e-6});

  const auto [value_v, vhp_lanes_out] = vhp_lanes(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction_lanes, HvpOptions{1e-6});

  REQUIRE(value_v.item<double>() ==
          Approx(value_h.item<double>()).margin(1e-12));
  REQUIRE(vhp_lanes_out.size() == hv_lanes_out.size());
  for (std::size_t lane = 0; lane < hv_lanes_out.size(); ++lane) {
    const auto h = hv_lanes_out[lane].to_vec<double>();
    const auto v = vhp_lanes_out[lane].to_vec<double>();
    REQUIRE(h.size() == v.size());
    for (std::size_t i = 0; i < h.size(); ++i) {
      REQUIRE(v[i] == Approx(h[i]).margin(1e-8));
    }
  }
}

TEST_CASE("gradient_audit validates directional first-order derivatives",
          "[autodiff][audit]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-6;
  opt.atol = 1e-6;
  opt.rtol = 1e-4;
  REQUIRE(gradient_audit(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, opt));
}

TEST_CASE("curvature_audit validates directional second-order derivatives",
          "[autodiff][audit]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-5;
  opt.atol = 5e-5;
  opt.rtol = 5e-3;
  opt.hvp.mode = HvpOptions::Mode::ExactOnly;
  REQUIRE(curvature_audit(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, opt));
}

TEST_CASE("gradient_audit rejects non-scalar objective outputs",
          "[autodiff][audit]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  REQUIRE_THROWS_WITH(
      gradient_audit([](const auto &z) { return vector_objective(z); }, x),
      Catch::Matchers::ContainsSubstring("scalar-valued"));
}

TEST_CASE("gradient_audit_report returns mismatch diagnostics",
          "[autodiff][audit]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-1;
  opt.atol = 0.0;
  opt.rtol = 0.0;
  const auto report = gradient_audit_report(
      [](const auto &z) {
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::sum;
        return sum(exp(z));
      },
      x, opt);
  REQUIRE_FALSE(report.passed);
  REQUIRE(report.phase == "gradient");
  REQUIRE(report.lane_index >= 0);
  REQUIRE(std::isfinite(report.analytic));
  REQUIRE(std::isfinite(report.numeric));
  REQUIRE(report.abs_error > report.tolerance);
}

TEST_CASE("curvature_audit_report returns mismatch diagnostics",
          "[autodiff][audit]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  DerivativeAuditOptions opt{};
  opt.epsilon = 1e-1;
  opt.atol = 0.0;
  opt.rtol = 0.0;
  opt.hvp.mode = HvpOptions::Mode::ExactOnly;
  const auto report = curvature_audit_report(
      [](const auto &z) {
        using pluss::subsp::math::diff::exp;
        using pluss::subsp::math::diff::sum;
        return sum(exp(z));
      },
      x, opt);
  REQUIRE_FALSE(report.passed);
  REQUIRE(report.phase == "curvature");
  REQUIRE(report.lane_index >= 0);
  REQUIRE(std::isfinite(report.analytic));
  REQUIRE(std::isfinite(report.numeric));
  REQUIRE(report.abs_error > report.tolerance);
}

TEST_CASE("hessian wrapper matches analytic scalar second derivative",
          "[autodiff][hessian]") {
  const Series x = make_series(1.25).to(ScalarType::Float64);
  const auto poly = [](const auto &z) { return z * z * z - 2.0 * (z * z) + z; };
  const auto [value, h] = hessian(poly, x);
  REQUIRE(value.item<double>() == Approx(0.078125).margin(1e-12));
  REQUIRE(h.item<double>() == Approx(3.5).margin(1e-6));
}

TEST_CASE(
    "hessian block form returns expected cross-curvature for scalar params",
    "[autodiff][hessian]") {
  const Series p = make_series(0.7).to(ScalarType::Float64);
  const Series q = make_series(-0.2).to(ScalarType::Float64);
  const std::vector<Series> params{p, q};

  const auto [value, blocks] = hessian(
      [](const std::vector<Trace> &x) -> Trace {
        const Trace a = x[0];
        const Trace b = x[1];
        return a * a + a * b + (2.0 * b * b);
      },
      params);

  REQUIRE(value.numel() == 1);
  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].size() == 2);
  REQUIRE(blocks[1].size() == 2);
  REQUIRE(blocks[0][0].item<double>() == Approx(2.0).margin(1e-6));
  REQUIRE(blocks[0][1].item<double>() == Approx(1.0).margin(1e-6));
  REQUIRE(blocks[1][0].item<double>() == Approx(1.0).margin(1e-6));
  REQUIRE(blocks[1][1].item<double>() == Approx(4.0).margin(1e-6));
}

TEST_CASE("hessian vectorized option preserves block semantics",
          "[autodiff][hessian]") {
  const Series p = make_series(0.7).to(ScalarType::Float64);
  const Series q = make_series(-0.2).to(ScalarType::Float64);
  const std::vector<Series> params{p, q};

  const auto objective = [](const std::vector<Trace> &x) -> Trace {
    const Trace a = x[0];
    const Trace b = x[1];
    return a * a + a * b + (2.0 * b * b);
  };

  HessianOptions base{};
  const auto [value_base, blocks_base] = hessian(objective, params, base);

  HessianOptions vec{};
  vec.vectorized = true;
  const auto [value_vec, blocks_vec] = hessian(objective, params, vec);

  REQUIRE(value_vec.item<double>() ==
          Approx(value_base.item<double>()).margin(1e-12));
  REQUIRE(blocks_base.size() == blocks_vec.size());
  for (std::size_t i = 0; i < blocks_base.size(); ++i) {
    REQUIRE(blocks_base[i].size() == blocks_vec[i].size());
    for (std::size_t j = 0; j < blocks_base[i].size(); ++j) {
      REQUIRE(blocks_vec[i][j].item<double>() ==
              Approx(blocks_base[i][j].item<double>()).margin(1e-8));
    }
  }
}

TEST_CASE("nth_diff delegates to first and second-order transforms",
          "[autodiff][nth]") {
  const Series x = make_series(1.25).to(ScalarType::Float64);
  const auto poly = [](const auto &z) { return z * z * z - 2.0 * (z * z) + z; };

  const Series value = nth_diff(poly, x, 0);
  const Series first = nth_diff(poly, x, 1);
  const Series second = nth_diff(poly, x, 2);

  REQUIRE(value.item<double>() == Approx(0.078125));
  REQUIRE(first.item<double>() == Approx(0.6875));
  REQUIRE(second.item<double>() == Approx(3.5));
}

TEST_CASE("nth_diff supports exact higher orders for scalar Trace objective",
          "[autodiff][nth]") {
  const Series x = make_series(1.5).to(ScalarType::Float64);
  const auto quartic = [](const auto &z) { return z * z * z * z; };

  const Series third = nth_diff(quartic, x, 3);
  REQUIRE(third.item<double>() == Approx(36.0).margin(1e-6));

  const Series fourth = nth_diff(quartic, x, 4);
  REQUIRE(fourth.item<double>() == Approx(24.0).margin(1e-6));

  const Series fifth = nth_diff(quartic, x, 5);
  REQUIRE(fifth.item<double>() == Approx(0.0).margin(1e-8));
}

TEST_CASE(
    "nth_diff composes product and transcendental rules through sixth order",
    "[autodiff][nth][composition]") {
  const double point = 0.37;
  const Series x = make_series(point).to(ScalarType::Float64);
  const auto product = [](const Series &z) { return z.exp().mul(z.sin()); };

  // d^n/dx^n [exp(x) sin(x)] = 2^(n/2) exp(x) sin(x + n*pi/4).
  for (std::int64_t order = 0; order <= 6; ++order) {
    const double expected =
        std::pow(2.0, 0.5 * static_cast<double>(order)) * std::exp(point) *
        std::sin(point + static_cast<double>(order) * std::numbers::pi / 4.0);
    REQUIRE(nth_diff(product, x, order).item<double>() ==
            Approx(expected).margin(2e-10));
  }
}

TEST_CASE(
    "nth_diff distinguishes constant Jacobians from detached nonlinear rules",
    "[autodiff][nth][contract]") {
  const Series x = make_series(-1.75).to(ScalarType::Float64);
  REQUIRE(
      nth_diff([](const Series &z) { return z.abs(); }, x, 1).item<double>() ==
      Approx(-1.0));
  REQUIRE(
      nth_diff([](const Series &z) { return z.abs(); }, x, 2).item<double>() ==
      Approx(0.0).margin(1e-12));

  const Series vector = make_f64({1.0, -2.0, 4.0});
  const Series third =
      nth_diff([](const Series &z) { return z.mean(); }, vector, 3);
  REQUIRE(third.sizes() == std::vector<std::int64_t>{3, 3, 3});
  REQUIRE(third.abs().sum().item<double>() == Approx(0.0).margin(1e-12));
}

TEST_CASE("nth_diff traces recurrence-based special-function reverse rules",
          "[autodiff][nth][special]") {
  const Series x = make_series(1.75).to(ScalarType::Float64);

  REQUIRE(nth_diff([](const Series &z) { return z.lgamma(); }, x, 2)
              .item<double>() ==
          Approx(x.polygamma(1).item<double>()).margin(2e-10));
  REQUIRE(nth_diff([](const Series &z) { return z.digamma(); }, x, 2)
              .item<double>() ==
          Approx(x.polygamma(2).item<double>()).margin(2e-10));
  REQUIRE(nth_diff([](const Series &z) { return z.polygamma(2); }, x, 3)
              .item<double>() ==
          Approx(x.polygamma(5).item<double>()).margin(2e-9));

  const Series multi_gamma_point = make_series(3.25).to(ScalarType::Float64);
  const double multi_gamma_second =
      multi_gamma_point.polygamma(1).item<double>() +
      multi_gamma_point.sub(0.5).polygamma(1).item<double>() +
      multi_gamma_point.sub(1.0).polygamma(1).item<double>();
  REQUIRE(nth_diff([](const Series &z) { return z.mvlgamma(3); },
                   multi_gamma_point, 2)
              .item<double>() == Approx(multi_gamma_second).margin(2e-9));

  const Series inverse_point = make_series(0.35).to(ScalarType::Float64);
  const double inverse = inverse_point.erfinv().item<double>();
  const double first =
      std::sqrt(std::numbers::pi) * 0.5 * std::exp(inverse * inverse);
  const double second = 2.0 * inverse * first * first;
  REQUIRE(nth_diff([](const Series &z) { return z.erfinv(); }, inverse_point, 2)
              .item<double>() == Approx(second).margin(2e-9));

  const Series bessel_point = make_series(1.2).to(ScalarType::Float64);
  const double i0 = bessel_point.i0().item<double>();
  const double i1 =
      nth_diff([](const Series &z) { return z.i0(); }, bessel_point, 1)
          .item<double>();
  REQUIRE(nth_diff([](const Series &z) { return z.i0(); }, bessel_point, 2)
              .item<double>() == Approx(i0 - i1 / 1.2).margin(2e-8));

  const Series origin = make_series(0.0).to(ScalarType::Float64);
  REQUIRE(nth_diff([](const Series &z) { return z.i0(); }, origin, 2)
              .item<double>() == Approx(0.5).margin(2e-10));
  REQUIRE(nth_diff([](const Series &z) { return z.i0(); }, origin, 4)
              .item<double>() == Approx(3.0 / 8.0).margin(2e-9));
}

TEST_CASE(
    "nth_diff traces two-input special functions through supported operands",
    "[autodiff][nth][special]") {
  const Series x = make_series(2.25).to(ScalarType::Float64);
  const Series a = make_series(3.5).to(ScalarType::Float64);

  const double igamma_first =
      std::exp((3.5 - 1.0) * std::log(2.25) - 2.25 - std::lgamma(3.5));
  const double igamma_second = igamma_first * ((3.5 - 1.0) / 2.25 - 1.0);
  REQUIRE(nth_diff([&a](const Series &z) { return a.igamma(z); }, x, 2)
              .item<double>() == Approx(igamma_second).margin(2e-9));

  const Series exponent = make_series(3.0).to(ScalarType::Float64);
  const double zeta_second =
      3.0 * 4.0 * exponent.add(2.0).zeta(x).item<double>();
  REQUIRE(
      nth_diff([&exponent](const Series &z) { return exponent.zeta(z); }, x, 2)
          .item<double>() == Approx(zeta_second).margin(2e-9));

  const Series beta_left = make_series(1.4).to(ScalarType::Float64);
  REQUIRE(std::isfinite(
      nth_diff([&beta_left](const Series &z) { return beta_left.beta(z); }, x,
               3)
          .item<double>()));
  REQUIRE(std::isfinite(
      nth_diff([&beta_left](const Series &z) { return beta_left.betaln(z); }, x,
               3)
          .item<double>()));
}

TEST_CASE("nth_diff fails before detached reverse rules can masquerade as zero",
          "[autodiff][nth][contract]") {
  const Series x = make_series(0.35).to(ScalarType::Float64);

  REQUIRE(std::isfinite(
      nth_diff([](const Series &z) { return z.sinc(); }, x, 1).item<double>()));
  REQUIRE_THROWS_WITH(nth_diff([](const Series &z) { return z.sinc(); }, x, 2),
                      Catch::Matchers::ContainsSubstring("sinc"));
}

TEST_CASE("nth_diff preserves full higher-order output and input axes",
          "[autodiff][nth]") {
  const Series x = make_series(std::vector<double>{-2.0, 0.5, 3.0}, {3})
                       .to(ScalarType::Float64);
  const auto cubic = [](const auto &z) { return z * z * z; };

  const Series first = nth_diff(cubic, x, 1);
  const Series second = nth_diff(cubic, x, 2);
  const Series third = nth_diff(cubic, x, 3);

  REQUIRE(first.sizes() == std::vector<std::int64_t>{3, 3});
  REQUIRE(second.sizes() == std::vector<std::int64_t>{3, 3, 3});
  REQUIRE(third.sizes() == std::vector<std::int64_t>{3, 3, 3, 3});
  REQUIRE(first.diagonal(0, 0, 1).allclose(make_f64({12.0, 0.75, 27.0}), 1e-12,
                                           1e-12));
  REQUIRE(second.select(0, 0).select(0, 0).select(0, 0).item<double>() ==
          Approx(-12.0).margin(1e-12));
  REQUIRE(second.select(0, 1).select(0, 1).select(0, 1).item<double>() ==
          Approx(3.0).margin(1e-12));
  REQUIRE(second.select(0, 2).select(0, 2).select(0, 2).item<double>() ==
          Approx(18.0).margin(1e-12));
  REQUIRE(second.sum().item<double>() == Approx(9.0).margin(1e-12));
  REQUIRE(third.select(0, 0)
              .select(0, 0)
              .select(0, 0)
              .select(0, 0)
              .item<double>() == Approx(6.0).margin(1e-12));
  REQUIRE(third.sum().item<double>() == Approx(18.0).margin(1e-12));
}

TEST_CASE("nth_diff retains mixed partials for vector mappings",
          "[autodiff][nth]") {
  const Series x =
      make_series(std::vector<double>{2.0, 3.0}, {2}).to(ScalarType::Float64);
  const auto coupled = [](const Series &z) {
    const auto x0 = z.select(0, 0);
    const auto x1 = z.select(0, 1);
    return x0.mul(x1).reshape({1}).unite({x0.square().add(x1).reshape({1})}, 0);
  };

  const Series second = nth_diff(coupled, x, 2);
  REQUIRE(second.sizes() == std::vector<std::int64_t>{2, 2, 2});
  REQUIRE(second.select(0, 0).allclose(
      make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0}, {2, 2})
          .to(ScalarType::Float64),
      1e-12, 1e-12));
  REQUIRE(second.select(0, 1).allclose(
      make_series(std::vector<double>{2.0, 0.0, 0.0, 0.0}, {2, 2})
          .to(ScalarType::Float64),
      1e-12, 1e-12));
}

TEST_CASE("nth_diff traces matrix-product reverse rules",
          "[autodiff][nth][matmul]") {
  const Series x = make_series(std::vector<double>{1.0, 2.0, -1.0, 3.0}, {2, 2})
                       .to(ScalarType::Float64);
  const auto squared_frobenius = [](const Series &z) {
    return z.matmul(z.transpose(0, 1)).sum();
  };

  const Series second = nth_diff(squared_frobenius, x, 2);
  REQUIRE(second.sizes() == std::vector<std::int64_t>{2, 2, 2, 2});

  const auto values = second.to_vec<double>();
  REQUIRE(values.size() == 16);
  const std::array<double, 16> expected{
      2.0, 0.0, 2.0, 0.0, 0.0, 2.0, 0.0, 2.0,
      2.0, 0.0, 2.0, 0.0, 0.0, 2.0, 0.0, 2.0,
  };
  for (std::size_t i = 0; i < values.size(); ++i) {
    REQUIRE(values[i] == Approx(expected[i]).margin(1e-10));
  }
}

TEST_CASE(
    "Series calc accessor delegates arbitrary order to the transform engine",
    "[autodiff][nth][accessor]") {
  const Series x = make_f64({-2.0, 0.5, 3.0});
  const Series via_dot =
      x.calc.diff([](const Series &z) { return z.pow(3.0); }, 2);
  const Series via_call =
      x.calc().diff([](const Series &z) { return z.pow(3.0); }, 2);

  REQUIRE(via_dot.sizes() == std::vector<std::int64_t>{3, 3, 3});
  REQUIRE(via_dot.equal(via_call));
  REQUIRE(via_dot.select(0, 0).select(0, 0).select(0, 0).item<double>() ==
          Approx(-12.0).margin(1e-12));
}

TEST_CASE("Series calc named derivatives enforce their mathematical shapes",
          "[autodiff][calc][gradient][jacobian][hessian]") {
  const Series x =
      make_series(std::vector<double>{2.0, 3.0}, std::vector<std::int64_t>{2})
          .to(ScalarType::Float64);

  SECTION("gradient") {
    const Series gradient =
        x.calc.grad([](const Series &z) { return z.square().sum(); });
    REQUIRE(gradient.sizes() == std::vector<std::int64_t>{2});
    REQUIRE(gradient.allclose(
        make_series(std::vector<double>{4.0, 6.0}, std::vector<std::int64_t>{2})
            .to(ScalarType::Float64)));
    REQUIRE_THROWS_WITH(x.calc.grad([](const Series &z) { return z.square(); }),
                        Catch::Matchers::ContainsSubstring("rank-zero scalar"));
  }

  SECTION("Jacobian") {
    const Series jacobian = x.calc.jac([](const Series &z) {
      return z.square().unite({z.sum().reshape({1})}, 0);
    });
    REQUIRE(jacobian.sizes() == std::vector<std::int64_t>{3, 2});
    REQUIRE(jacobian.allclose(
        make_series(std::vector<double>{4.0, 0.0, 0.0, 6.0, 1.0, 1.0},
                    std::vector<std::int64_t>{3, 2})
            .to(ScalarType::Float64)));
  }

  SECTION("Hessian") {
    const Series hessian =
        x.calc.hess([](const Series &z) { return z.pow(3.0).sum(); });
    REQUIRE(hessian.sizes() == std::vector<std::int64_t>{2, 2});
    REQUIRE(
        hessian.allclose(make_series(std::vector<double>{12.0, 0.0, 0.0, 18.0},
                                     std::vector<std::int64_t>{2, 2})
                             .to(ScalarType::Float64)));
    REQUIRE_THROWS_WITH(x.calc.hess([](const Series &z) { return z.square(); }),
                        Catch::Matchers::ContainsSubstring("rank-zero scalar"));
  }
}

TEST_CASE("Series calc named derivatives preserve an available accelerator",
          "[autodiff][calc][accelerator]") {
  for (const auto platform : {pluss::pMPS, pluss::pCUDA}) {
    if (!pluss::platform_available(platform))
      continue;
    // MPS has no float64, so run it in float32 and compare with a float32 tolerance.
    const ScalarType dtype =
        platform == pluss::pMPS ? ScalarType::Float32 : ScalarType::Float64;
    const auto host = [&](std::vector<double> v, std::vector<std::int64_t> shape) {
      return make_series(std::move(v), std::move(shape)).to(dtype);
    };
    const Series x = host({2.0, 3.0}, {2}).to(platform);
    const Series gradient =
        x.calc.grad([](const Series &z) { return z.square().sum(); });
    const Series jacobian =
        x.calc.jac([](const Series &z) { return z.square(); });
    const Series hessian =
        x.calc.hess([](const Series &z) { return z.pow(3.0).sum(); });

    REQUIRE(gradient.platform() == platform);
    REQUIRE(jacobian.platform() == platform);
    REQUIRE(hessian.platform() == platform);
    REQUIRE(gradient.to(pluss::pCPU).allclose(host({4.0, 6.0}, {2}), 1e-5, 1e-6));
    REQUIRE(jacobian.to(pluss::pCPU)
                .allclose(host({4.0, 0.0, 0.0, 6.0}, {2, 2}), 1e-5, 1e-6));
    REQUIRE(hessian.to(pluss::pCPU)
                .allclose(host({12.0, 0.0, 0.0, 18.0}, {2, 2}), 1e-5, 1e-6));
  }
}

TEST_CASE("Series calc antidiff evaluates anchored repeated integrals",
          "[autodiff][calc][antidiff]") {
  Series::CalcAccessor::AntidiffOptions options{};
  options.intervals = 32;
  options.adaptive = false;

  const Series x = make_f64({0.5, 1.0, 2.0});
  const Series first =
      x.calc.antidiff([](const Series &t) { return t.square(); }, 0.0, options);
  const auto first_values = first.to_vec<double>();
  REQUIRE(first_values[0] == Approx(0.125 / 3.0).margin(1e-12));
  REQUIRE(first_values[1] == Approx(1.0 / 3.0).margin(1e-12));
  REQUIRE(first_values[2] == Approx(8.0 / 3.0).margin(1e-12));

  options.order = 2;
  options.initial = {2.0, 3.0};
  const Series second = x.calc().antidiff(
      [](const Series &t) { return t.mul(0.0).add(1.0); }, 0.0, options);
  const auto second_values = second.to_vec<double>();
  REQUIRE(second_values[0] == Approx(3.625).margin(1e-12));
  REQUIRE(second_values[1] == Approx(5.5).margin(1e-12));
  REQUIRE(second_values[2] == Approx(10.0).margin(1e-12));

  Series endpoint = make_series(0.75).to(ScalarType::Float64);
  endpoint.tracks_slope_(true);
  options.order = 1;
  options.initial.clear();
  const Series integral = endpoint.calc.antidiff(
      [](const Series &t) { return t.square(); }, 0.0, options);
  integral.reverse();
  REQUIRE(endpoint.sensitivity().item<double>() ==
          Approx(0.75 * 0.75).margin(1e-10));

  Series::CalcAccessor::AntidiffOptions adaptive{};
  adaptive.intervals = 8;
  const Series pi = make_series(std::numbers::pi).to(ScalarType::Float64);
  REQUIRE(
      pi.calc.antidiff([](const Series &t) { return t.sin(); }, 0.0, adaptive)
          .item<double>() == Approx(2.0).margin(1e-9));

  Series::CalcAccessor::AntidiffOptions trapezoid{};
  trapezoid.rule = Series::CalcAccessor::Rule::Trapezoid;
  trapezoid.intervals = 16;
  trapezoid.adaptive = false;
  const Series reversed = make_series(-1.0).to(ScalarType::Float64);
  REQUIRE(
      reversed.calc.antidiff([](const Series &t) { return t; }, 0.0, trapezoid)
          .item<double>() == Approx(0.5).margin(1e-12));

  options.order = 2;
  options.initial = {1.0};
  REQUIRE_THROWS_WITH(
      x.calc.antidiff([](const Series &t) { return t; }, 0.0, options),
      Catch::Matchers::ContainsSubstring("one value per order"));

  options.initial = {0.0, 0.0};
  options.rule = static_cast<Series::CalcAccessor::Rule>(255);
  REQUIRE_THROWS_WITH(
      x.calc.antidiff([](const Series &t) { return t; }, 0.0, options),
      Catch::Matchers::ContainsSubstring("rule is not recognized"));
}

TEST_CASE("Series calc accessor is bound on values straight from a factory",
          "[autodiff][calc][accessor]") {
  // Guaranteed elision builds these in place, so the accessors must be bound by
  // the internal defined-state constructor, not only by the copy/move ones.
  const Series x = pluss::series::full({}, 1.7, ScalarType::Float64);
  REQUIRE(x.calc.diff([](const Series &z) { return z.sin(); }).item<double>() ==
          Approx(std::cos(1.7)).margin(1e-12));
  REQUIRE(pluss::series::full({}, 1.7, ScalarType::Float64)
              .calc.antidiff([](const Series &t) { return t.cos(); }, 0.0)
              .item<double>() == Approx(std::sin(1.7)).margin(1e-8));
  REQUIRE(pluss::series::full({3}, 1.0, ScalarType::Float64)
              .antidiff.trapezoid(1.0)
              .item<double>() == Approx(2.0).margin(1e-12));
}

TEST_CASE("Series calc arguments bind once and explicit arguments win",
          "[autodiff][calc][options]") {
  using Opts = Series::CalcAccessor::CalcOptions;
  const Series x = pluss::series::full({}, 1.7, ScalarType::Float64);
  const double a = 0.3;
  auto neg_sin = [](const Series &t) { return t.sin().mul(-1.0); };

  // Second-order round trip: diff then antidiff with the right initial
  // conditions.
  Opts back;
  back.order = 2;
  back.lower = a;
  back.initial = {std::sin(a), std::cos(a)};
  const Series f_back = x.calc(back).antidiff(neg_sin);
  REQUIRE(f_back.sizes() == x.sizes()); // a 0-d endpoint stays 0-d
  REQUIRE(f_back.item<double>() == Approx(std::sin(1.7)).margin(1e-8));

  // Zero initial conditions return f minus its Taylor polynomial about `lower`.
  Opts zero = back;
  zero.initial.clear();
  REQUIRE(x.calc(zero).antidiff(neg_sin).item<double>() ==
          Approx(std::sin(1.7) - std::sin(a) - std::cos(a) * (1.7 - a))
              .margin(1e-8));

  // diff(antidiff(g)) recovers g.
  Opts first;
  first.lower = a;
  const auto F = [&](const Series &u) {
    return u.calc(first).antidiff([](const Series &t) { return t.cos(); });
  };
  REQUIRE(x.calc.diff(F).item<double>() == Approx(std::cos(1.7)).margin(1e-7));

  // An explicit lower bound overrides the bound one; a missing one is an error.
  REQUIRE(x.calc(first)
              .antidiff([](const Series &t) { return t.cos(); }, 0.0)
              .item<double>() == Approx(std::sin(1.7)).margin(1e-8));
  REQUIRE_THROWS_WITH(x.calc.antidiff([](const Series &t) { return t.cos(); }),
                      Catch::Matchers::ContainsSubstring("no lower bound"));

  // diff's order: bound option, explicit argument, and the default of one.
  Opts second;
  second.order = 2;
  auto sine = [](const Series &z) { return z.sin(); };
  REQUIRE(x.calc(second).diff(sine).item<double>() ==
          Approx(-std::sin(1.7)).margin(1e-12));
  REQUIRE(x.calc(second).diff(sine, 1).item<double>() ==
          Approx(std::cos(1.7)).margin(1e-12));
  REQUIRE(x.calc.diff(sine, 0).item<double>() ==
          Approx(std::sin(1.7)).margin(1e-12));
}

TEST_CASE("Series calc elementwise derivatives keep the shape of x",
          "[autodiff][calc][elementwise]") {
  using Opts = Series::CalcAccessor::CalcOptions;
  const std::vector<double> pts{0.5, 1.0, 1.7, 2.4};
  const Series x = make_series(pts, {4}).to(ScalarType::Float64);
  auto sine = [](const Series &z) { return z.sin(); };

  Opts el;
  el.elementwise = true;
  for (std::int64_t order = 1; order <= 4; ++order) {
    const Series d = x.calc(el).diff(sine, order);
    REQUIRE(d.sizes() == x.sizes());
    const auto got = d.to_vec<double>();
    for (std::size_t i = 0; i < pts.size(); ++i) {
      const double want = order % 4 == 1   ? std::cos(pts[i])
                          : order % 4 == 2 ? -std::sin(pts[i])
                          : order % 4 == 3 ? -std::cos(pts[i])
                                           : std::sin(pts[i]);
      REQUIRE(got[i] == Approx(want).margin(1e-10));
    }
  }
  // The full-tensor default is the diagonal of the same thing.
  const Series full = x.calc.diff(sine);
  REQUIRE(full.sizes() == std::vector<std::int64_t>{4, 4});
  REQUIRE(full.select(0, 2).select(0, 2).item<double>() ==
          Approx(std::cos(1.7)).margin(1e-12));

  // Antidiff of a vector of endpoints is one value per endpoint, matching
  // elementwise diff.
  Opts anchor;
  anchor.lower = 0.3;
  anchor.initial = {std::sin(0.3)};
  const Series F =
      x.calc(anchor).antidiff([](const Series &t) { return t.cos(); });
  REQUIRE(F.sizes() == x.sizes());
  const auto back = F.to_vec<double>();
  for (std::size_t i = 0; i < pts.size(); ++i) {
    REQUIRE(back[i] == Approx(std::sin(pts[i])).margin(1e-8));
  }

  // Elementwise mode refuses a function that changes the shape, and strict mode
  // refuses a constant.
  REQUIRE_THROWS_WITH(x.calc(el).diff([](const Series &z) { return z.sum(); }),
                      Catch::Matchers::ContainsSubstring("shape of x"));
  Opts strict = el;
  strict.strict_dependency = true;
  REQUIRE_THROWS(x.calc(strict).diff(
      [](const Series &z) { return z.mul(0.0).add(1.0).detach(); }));
}

TEST_CASE(
    "Series calc antidiff names a singular integrand instead of reporting nan",
    "[autodiff][calc][antidiff]") {
  const Series x = pluss::series::full({}, 1.0, ScalarType::Float64);
  REQUIRE_THROWS_WITH(
      x.calc.antidiff([](const Series &t) { return t.abs().pow(-0.5); }, -1.0),
      Catch::Matchers::ContainsSubstring("non-finite"));
}

TEST_CASE(
    "nth_diff packs multiple independent Series into one mixed-partial space",
    "[autodiff][nth][params]") {
  const Series a =
      make_series(std::vector<double>{2.0, 3.0}, {2}).to(ScalarType::Float64);
  const Series b = make_series(5.0)
                       .to(ScalarType::Float32)
                       .reshape(std::vector<std::int64_t>{});
  const std::vector<Series> params{a, b};

  const auto fn = [](const std::vector<Series> &xs) {
    const Series a0 = xs[0].select(0, 0);
    const Series a1 = xs[0].select(0, 1);
    const Series b0 = xs[1];
    return a0.square().mul(b0).add(a1.mul(b0.square()));
  };

  const Series second = nth_diff(fn, params, 2);
  // Scalar objective: output shape () + input shape (3 packed parameters) twice.
  // (An earlier expectation of {1, 3, 3} encoded a bug where scalar-constant
  // arithmetic promoted 0-d results to rank 1.)
  REQUIRE(second.sizes() == std::vector<std::int64_t>{3, 3});
  const std::array<double, 9> expected{
      10.0, 0.0, 4.0, 0.0, 0.0, 10.0, 4.0, 10.0, 6.0,
  };
  const auto values = second.to_vec<double>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    REQUIRE(values[i] == Approx(expected[i]).margin(1e-10));
  }

  const Series third = nth_diff(fn, params, 3);
  REQUIRE(third.sizes() == std::vector<std::int64_t>{3, 3, 3});
  REQUIRE(third.select(0, 0).select(0, 0).select(0, 2).item<double>() ==
          Approx(2.0).margin(1e-10));
  REQUIRE(third.select(0, 1).select(0, 2).select(0, 2).item<double>() ==
          Approx(2.0).margin(1e-10));

  REQUIRE_THROWS_WITH(
      nth_diff(fn, std::vector<Series>{}, 2),
      Catch::Matchers::ContainsSubstring("at least one independent Series"));
  const Series integer_param =
      pluss::series::arange(0, 2, 1, ScalarType::Int64);
  REQUIRE_THROWS_WITH(
      nth_diff(fn, std::vector<Series>{integer_param, b}, 2),
      Catch::Matchers::ContainsSubstring("floating or complex dtype"));
}

TEST_CASE("nth_diff defines empty tensors and guards impossible tensor ranks",
          "[autodiff][nth][contract]") {
  const Series empty = pluss::series::empty({0, 2}, ScalarType::Float64);
  const Series derivative =
      nth_diff([](const Series &z) { return z.square(); }, empty, 2);
  REQUIRE(derivative.sizes() == std::vector<std::int64_t>{0, 2, 0, 2, 0, 2});
  REQUIRE(derivative.numel() == 0);

  const Series one =
      make_series(std::vector<double>{1.0}, {1, 1}).to(ScalarType::Float64);
  REQUIRE_THROWS_WITH(nth_diff([](const Series &z) { return z; }, one, 32),
                      Catch::Matchers::ContainsSubstring("rank exceeds"));
  REQUIRE_THROWS_WITH(nth_diff([](const Series &z) { return z; }, one, -1),
                      Catch::Matchers::ContainsSubstring("non-negative"));
}

TEST_CASE("vector parameter gradients preserve order and shape",
          "[autodiff][params]") {
  std::vector<Series> params{
      make_series(0.25).to(ScalarType::Float64),
      make_series(-0.1).to(ScalarType::Float64),
  };
  const Series x = pluss::series::linspace(-1.0, 1.0, 16, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  const auto [value, grads] = value_and_grad(
      [&](const std::vector<Trace> &p) -> Trace {
        const auto pred = p.at(0) * trace(x, false) + p.at(1);
        const auto err = pred - trace(y, false);
        return mean(err * err);
      },
      params);

  REQUIRE(value.numel() == 1);
  REQUIRE(grads.size() == 2);
  REQUIRE(grads[0].numel() == 1);
  REQUIRE(grads[1].numel() == 1);
  REQUIRE(std::isfinite(grads[0].item<double>()));
  REQUIRE(std::isfinite(grads[1].item<double>()));
}

TEST_CASE("transform validation rejects invalid tangent shape",
          "[autodiff][validation]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series bad_tangent =
      make_series(std::vector<double>{1.0, -1.0}, {2}).to(ScalarType::Float64);
  REQUIRE_THROWS(
      jvp([](const DualTrace &z) -> DualTrace { return vector_objective(z); },
          x, bad_tangent));
}

TEST_CASE("hvp validation rejects non-scalar objectives",
          "[autodiff][validation]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  REQUIRE_THROWS(
      hvp([](const auto &z) { return vector_objective(z); }, x, tangent));
}

TEST_CASE("hvp exact/fd mode policy is explicit and fail-loud",
          "[autodiff][hvp][policy]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});

  HvpOptions exact_opt{};
  exact_opt.mode = HvpOptions::Mode::ExactOnly;
  const auto [value_exact, hv_exact] = hvp(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, tangent, exact_opt);
  REQUIRE(value_exact.numel() == 1);
  REQUIRE(hv_exact.numel() == 3);

  HvpOptions fd_only_opt{};
  fd_only_opt.mode = HvpOptions::Mode::FiniteDifferenceOnly;
  REQUIRE_THROWS(hvp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, tangent, fd_only_opt));

  REQUIRE_THROWS(hvp(
      [](const Trace &z) -> Trace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, tangent, exact_opt));
}

TEST_CASE("vhp mode policy mirrors hvp fail-loud behavior",
          "[autodiff][vhp][policy]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series direction = make_f64({0.1, 0.05, -0.2});

  HvpOptions exact_opt{};
  exact_opt.mode = HvpOptions::Mode::ExactOnly;
  const auto [value_exact, vh_exact] = vhp(
      [](const auto &z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction, exact_opt);
  REQUIRE(value_exact.numel() == 1);
  REQUIRE(vh_exact.numel() == 3);

  HvpOptions fd_only_opt{};
  fd_only_opt.mode = HvpOptions::Mode::FiniteDifferenceOnly;
  REQUIRE_THROWS(vhp(
      [](const DualTrace &z) -> DualTrace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction, fd_only_opt));

  REQUIRE_THROWS(vhp(
      [](const Trace &z) -> Trace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x, direction, exact_opt));
}

TEST_CASE("reverse diagnostics include context on seeded reverse failures",
          "[autodiff][diagnostics]") {
  Series x = make_f64({0.3, -0.2, 0.5});
  Trace tx = trace(x, true);
  Trace y = tx * tx;
  const Series bad_seed =
      make_series(std::vector<double>{1.0, -1.0}, {2}).to(ScalarType::Float64);

  bool threw = false;
  std::string message;
  try {
    y.reverse(bad_seed);
  } catch (const std::exception &e) {
    threw = true;
    message = e.what();
  }
  REQUIRE(threw);
  REQUIRE(message.find("Trace::reverse") != std::string::npos);
  REQUIRE(message.find("hint") != std::string::npos);
}

TEST_CASE("autodiff remains finite under repeated nonlinear training steps",
          "[autodiff][stability]") {
  const Series x = pluss::series::linspace(-2.0, 2.0, 128, ScalarType::Float64);
  const Series y = x.mul(1.8).add(0.25).tanh();

  std::vector<Series> params{
      make_series(0.1).to(ScalarType::Float64),
      make_series(-0.2).to(ScalarType::Float64),
  };

  for (int i = 0; i < 120; ++i) {
    const auto [loss, grads] = value_and_grad(
        [&](const std::vector<Trace> &p) -> Trace {
          const Trace pred = tanh(p[0] * trace(x, false) + p[1]);
          const Trace err = pred - trace(y, false);
          return mean(square(err)) + 1e-3 * (square(p[0]) + square(p[1]));
        },
        params);

    REQUIRE(std::isfinite(loss.item<double>()));
    REQUIRE(std::isfinite(grads[0].item<double>()));
    REQUIRE(std::isfinite(grads[1].item<double>()));

    const double lr = 0.03;
    params[0] = params[0].sub(grads[0].mul(lr));
    params[1] = params[1].sub(grads[1].mul(lr));
  }
}
