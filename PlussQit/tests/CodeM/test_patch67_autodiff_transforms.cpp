#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::HvpOptions;
using pluss::subsp::math::diff::ReverseDebugScope;
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::jacfwd;
using pluss::subsp::math::diff::jacrev;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_derivative;
using pluss::subsp::math::diff::slope_source;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::value_and_grad;
using pluss::subsp::math::diff::vjp;
using Catch::Approx;

Series make_f64(std::vector<double> data) {
  return make_series(std::move(data), {3}).to(ScalarType::Float64);
}

std::vector<double> analytic_slope(const std::vector<double>& x) {
  std::vector<double> out;
  out.reserve(x.size());
  for (double v : x) {
    out.push_back(std::cos(v) + 2.0 * v);
  }
  return out;
}

template <class X>
auto vector_objective(const X& x) {
  using pluss::subsp::math::diff::sin;
  using pluss::subsp::math::diff::square;
  return sin(x) + square(x);
}

template <class X>
auto scalar_objective(const X& x) {
  using pluss::subsp::math::diff::exp;
  using pluss::subsp::math::diff::log;
  using pluss::subsp::math::diff::mean;
  using pluss::subsp::math::diff::square;
  using pluss::subsp::math::diff::sum;
  return mean(square(x)) + sum(log(exp(x) + 1.0));
}

Series scalar_objective(const Series& x) {
  return x.square().mean().add(x.exp().add(1.0).log().sum());
}

double scalar_value(const Series& x) {
  return scalar_objective(x).item<double>();
}

} // namespace

TEST_CASE("autodiff reverse preserves scalar identity reductions", "[autodiff][reverse]") {
  Series x = make_series(1.25).to(ScalarType::Float64);
  x.tracks_slope_(true);

  Series loss = x.square().sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(2.5));
}

TEST_CASE("autodiff reverse propagates select sensitivities to parent", "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(ScalarType::Float64);
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

TEST_CASE("autodiff reverse propagates take sensitivities with duplicate indices", "[autodiff][reverse]") {
  Series x = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4}).to(ScalarType::Float64);
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

TEST_CASE("autodiff reverse seed shape mismatch fails loudly", "[autodiff][diagnostics]") {
  Series x = make_f64({0.3, -0.2, 0.5});
  x.tracks_slope_(true);
  const Series output = x.square();
  const Series bad_seed = make_series(1.0).to(ScalarType::Float64);
  REQUIRE_THROWS_WITH(
      output.reverse(bad_seed),
      Catch::Matchers::ContainsSubstring("seed shape"));
}

TEST_CASE("autodiff reverse debug catches non-finite sensitivity propagation", "[autodiff][diagnostics]") {
  Series x = make_series(0.0).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.log();
  ReverseDebugScope debug_scope{};
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("non-finite sensitivity"));
}

TEST_CASE("autodiff reverse reports operator context on saved-value mutation", "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series loss = x.mul(x).sum();
  x.storage().bump_version();
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("[op=mul,"));
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("captured_at="));
}

TEST_CASE("autodiff reverse reports gather operator in saved-value mutation diagnostics", "[autodiff][diagnostics]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {2, 2}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series idx = make_series(std::vector<std::int64_t>{0, 1, 1, 0}, {2, 2});
  const Series loss = x.gather(1, idx).sum();
  x.storage().bump_version();
  REQUIRE_THROWS_WITH(
      loss.reverse(),
      Catch::Matchers::ContainsSubstring("[op=gather,"));
}

TEST_CASE("autodiff advanced index fails loudly when used on tracked tensors", "[autodiff][diagnostics]") {
  Series x = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series idx = make_series(std::vector<std::int64_t>{3, 1, 0}, {3});
  REQUIRE_THROWS_WITH(
      x.index(std::vector<Series>{idx}),
      Catch::Matchers::ContainsSubstring("reverse-mode sensitivity for advanced indexing is not implemented"));
}

TEST_CASE("autodiff slope_source reports captured op metadata", "[autodiff][diagnostics]") {
  Series x = make_series(1.5).to(ScalarType::Float64);
  x.tracks_slope_(true);
  const Series y = x.mul(x);
  const auto src = slope_source(y);
  REQUIRE(src.linked);
  REQUIRE(src.op == "mul");
  REQUIRE(src.parents == 2);
  REQUIRE_FALSE(src.capture_file.empty());
}

TEST_CASE("autodiff tracked batched matmul reverse matches finite difference", "[autodiff][reverse]") {
  std::vector<double> a_data{
      1.0, 2.0, 3.0, 4.0,
      5.0, 6.0, 7.0, 8.0};
  const std::vector<double> b_data{
      1.5, -0.5, 0.2, 0.8,
      -1.0, 0.3, 0.7, -0.4};
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

TEST_CASE("autodiff tracked broadcasted batched matmul reverse reduces correctly", "[autodiff][reverse]") {
  std::vector<double> a_data{
      1.0, 2.0,
      3.0, 4.0};
  const std::vector<double> b_data{
      1.5, -0.5, 0.2, 0.8,
      -1.0, 0.3, 0.7, -0.4};
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

TEST_CASE("autodiff tracked vector-matrix matmul reverse matches finite difference", "[autodiff][reverse]") {
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

TEST_CASE("autodiff tracked matrix-vector matmul reverse matches finite difference", "[autodiff][reverse]") {
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

TEST_CASE("autodiff tracked vector-vector matmul reverse matches finite difference", "[autodiff][reverse]") {
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

TEST_CASE("autodiff scatter_add propagates sensitivities to base and source", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4}, {4}).to(ScalarType::Float64);
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
  for (double v : gb) REQUIRE(v == Approx(1.0));
  for (double v : gs) REQUIRE(v == Approx(1.0));
}

TEST_CASE("autodiff scatter propagates sensitivities to base and source", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{5.0, 6.0}, {2}).to(ScalarType::Float64);
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

TEST_CASE("autodiff scatter_add scalar source sensitivity equals contribution count", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{2.0}, {1}).to(ScalarType::Float64);
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
  for (double v : gb) REQUIRE(v == Approx(1.0));
  REQUIRE(src.sensitivity().item<double>() == Approx(4.0));
}

TEST_CASE("autodiff scatter_reduce(sum) propagates sensitivities", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{0.5, -1.0, 2.0, 1.5}, {4}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 2, 2, 3}, {4});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter_reduce(0, idx, src, "sum", /*include_self=*/true);
  const Series loss = out.sum();
  loss.reverse();

  const auto gb = base.sensitivity().to_vec<double>();
  const auto gs = src.sensitivity().to_vec<double>();
  REQUIRE(gb.size() == 4);
  REQUIRE(gs.size() == 4);
  for (double v : gb) REQUIRE(v == Approx(1.0));
  for (double v : gs) REQUIRE(v == Approx(1.0));
}

TEST_CASE("autodiff scatter_reduce(sum, include_self=false) zeros base sensitivity at scattered indices", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{7.0, 8.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter_reduce(0, idx, src, "sum", /*include_self=*/false);
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

TEST_CASE("autodiff scatter_reduce fail-loud for unsupported tracked reduce mode", "[autodiff][reverse][diagnostics]") {
  Series base = make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{5.0, 6.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  REQUIRE_THROWS(base.scatter_reduce(0, idx, src, "prod", /*include_self=*/true));
}

TEST_CASE("autodiff scatter_reduce(amax) distributes ties across base and source", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{2.0, 5.0, 5.0, 1.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{5.0, 4.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 3}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter_reduce(0, idx, src, "amax", /*include_self=*/true);
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

TEST_CASE("autodiff scatter_reduce(amin, include_self=false) propagates only through source", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{1.0, 2.0}, {2}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{-3.0, 0.5}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{0, 1}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter_reduce(0, idx, src, "amin", /*include_self=*/false);
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

TEST_CASE("scatter_reduce(include_self=false) keeps untouched self lanes", "[autodiff][reverse]") {
  const Series base = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4}).to(ScalarType::Float64);
  const Series src = make_series(std::vector<double>{1.0, 3.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 1}, {2});

  const Series out = base.scatter_reduce(0, idx, src, "sum", /*include_self=*/false);
  const auto v = out.to_vec<double>();
  REQUIRE(v.size() == 4);
  REQUIRE(v[0] == Approx(10.0));
  REQUIRE(v[1] == Approx(4.0));
  REQUIRE(v[2] == Approx(30.0));
  REQUIRE(v[3] == Approx(40.0));
}

TEST_CASE("autodiff scatter_reduce(mean) gradients match count-normalized rules", "[autodiff][reverse]") {
  Series base = make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4}).to(ScalarType::Float64);
  Series src = make_series(std::vector<double>{1.0, 3.0}, {2}).to(ScalarType::Float64);
  const Series idx = make_series(std::vector<std::int64_t>{1, 1}, {2});
  base.tracks_slope_(true);
  src.tracks_slope_(true);

  const Series out = base.scatter_reduce(0, idx, src, "mean", /*include_self=*/true);
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

TEST_CASE("value_and_grad matches analytic and finite-difference gradients", "[autodiff][grad]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto [value, g] = value_and_grad(
      [](const Trace& z) -> Trace { return scalar_objective(z); },
      x);

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
  const auto [value, pullback] = vjp(
      [](const Trace& z) -> Trace { return vector_objective(z); },
      x);
  const Series pulled = pullback(cotangent);

  REQUIRE(value.sizes()[0] == 3);

  const auto expected_slope = analytic_slope(x.to_vec<double>());
  const auto c = cotangent.to_vec<double>();
  const auto got = pulled.to_vec<double>();
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == Approx(expected_slope[i] * c[i]).margin(1e-12));
  }
}

TEST_CASE("jvp matches exact vector-output directional derivative", "[autodiff][jvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, directional] = jvp(
      [](const DualTrace& z) -> DualTrace { return vector_objective(z); },
      x,
      tangent);

  REQUIRE(value.sizes()[0] == 3);

  const auto slope = analytic_slope(x.to_vec<double>());
  const auto t = tangent.to_vec<double>();
  const auto got = directional.to_vec<double>();
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == Approx(slope[i] * t[i]).margin(1e-12));
  }
}

TEST_CASE("expanded unary derivative coverage remains analytic-accurate", "[autodiff][coverage]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, directional] = jvp(
      [](const DualTrace& z) -> DualTrace {
        using pluss::subsp::math::diff::acos;
        using pluss::subsp::math::diff::asin;
        using pluss::subsp::math::diff::atan;
        using pluss::subsp::math::diff::cosh;
        using pluss::subsp::math::diff::erf;
        using pluss::subsp::math::diff::expm1;
        using pluss::subsp::math::diff::log1p;
        using pluss::subsp::math::diff::sinh;
        return asin(z * 0.2) + acos(z * 0.2) + atan(z) + sinh(z) + cosh(z) +
               log1p(z + 2.0) + expm1(z * 0.1) + erf(z * 0.3);
      },
      x,
      tangent);

  REQUIRE(value.sizes()[0] == 3);
  const auto xv = x.to_vec<double>();
  const auto tv = tangent.to_vec<double>();
  const auto dv = directional.to_vec<double>();
  constexpr double kTwoOverSqrtPi = 1.12837916709551257390;
  for (std::size_t i = 0; i < dv.size(); ++i) {
    const double s = xv[i];
    const double slope =
        0.2 / std::sqrt(1.0 - (0.2 * s) * (0.2 * s)) +
        (-0.2 / std::sqrt(1.0 - (0.2 * s) * (0.2 * s))) +
        1.0 / (1.0 + s * s) +
        std::cosh(s) +
        std::sinh(s) +
        1.0 / (s + 3.0) +
        0.1 * std::exp(0.1 * s) +
        0.3 * kTwoOverSqrtPi * std::exp(-(0.3 * s) * (0.3 * s));
    REQUIRE(dv[i] == Approx(slope * tv[i]).margin(1e-10));
  }
}

TEST_CASE("softmax and log_softmax directional derivatives are analytic-accurate", "[autodiff][coverage]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});

  const auto [softmax_value, softmax_dir] = jvp(
      [](const DualTrace& z) -> DualTrace {
        using pluss::subsp::math::diff::softmax;
        return softmax(z, 0);
      },
      x,
      tangent);
  const auto [logsoftmax_value, logsoftmax_dir] = jvp(
      [](const DualTrace& z) -> DualTrace {
        using pluss::subsp::math::diff::log_softmax;
        return log_softmax(z, 0);
      },
      x,
      tangent);

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

TEST_CASE("loss helper transforms match analytic first-order derivatives", "[autodiff][loss]") {
  const Series pred = make_f64({0.3, -0.2, 0.5});
  const Series target = make_f64({0.1, -0.4, 0.9});

  const auto [mse_value, mse_grad] = value_and_grad(
      [&](const Trace& p) -> Trace {
        using pluss::subsp::math::diff::squared_error_loss;
        return squared_error_loss(p, target);
      },
      pred);

  const auto [l1_value, l1_grad] = value_and_grad(
      [&](const Trace& p) -> Trace {
        using pluss::subsp::math::diff::absolute_error_loss;
        return absolute_error_loss(p, target);
      },
      pred);

  const auto [huber_value, huber_grad] = value_and_grad(
      [&](const Trace& p) -> Trace {
        using pluss::subsp::math::diff::huber_error_loss;
        return huber_error_loss(p, target, 0.25);
      },
      pred);

  const auto [logit_value, logit_grad] = value_and_grad(
      [&](const Trace& p) -> Trace {
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
      REQUIRE(huber_g[i] == Approx((err > 0.0 ? 0.25 : -0.25) / n).margin(1e-12));
    }
    REQUIRE(mse_g[i] == Approx((2.0 * err) / n).margin(1e-12));
    REQUIRE(l1_g[i] == Approx((err > 0.0 ? 1.0 : -1.0) / n).margin(1e-12));

    const double sig = 1.0 / (1.0 + std::exp(-pv[i]));
    const double term = (1.0 - tv[i]) * std::log1p(std::exp(pv[i])) + tv[i] * std::log1p(std::exp(-pv[i]));
    logit_expected += term;
    REQUIRE(logit_g[i] == Approx((sig - tv[i]) / n).margin(1e-12));
  }

  REQUIRE(mse_value.item<double>() == Approx(mse_expected / n).margin(1e-12));
  REQUIRE(l1_value.item<double>() == Approx(l1_expected / n).margin(1e-12));
  REQUIRE(huber_value.item<double>() == Approx(huber_expected / n).margin(1e-12));
  REQUIRE(logit_value.item<double>() == Approx(logit_expected / n).margin(1e-12));
}

TEST_CASE("linearize reuses a stable pushforward closure", "[autodiff][linearize]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto [value, pushforward] = linearize(
      [](const DualTrace& z) -> DualTrace { return vector_objective(z); },
      x);

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

TEST_CASE("jacrev and jacfwd agree with analytic Jacobian", "[autodiff][jacobian]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const auto objective = [](const auto& z) { return vector_objective(z); };

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

TEST_CASE("hvp matches analytic Hessian-vector product for scalar objective", "[autodiff][hvp]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  const auto [value, hv] = hvp(
      [](const auto& z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      tangent,
      HvpOptions{1e-6});

  REQUIRE(value.numel() == 1);

  const auto xv = x.to_vec<double>();
  const auto tv = tangent.to_vec<double>();
  const auto hvv = hv.to_vec<double>();
  for (std::size_t i = 0; i < hvv.size(); ++i) {
    const double expected = (2.0 - std::sin(xv[i])) * tv[i];
    REQUIRE(hvv[i] == Approx(expected).margin(1e-5));
  }
}

TEST_CASE("nth_derivative delegates to first and second-order transforms", "[autodiff][nth]") {
  const Series x = make_series(1.25).to(ScalarType::Float64);
  const auto poly = [](const auto& z) { return z * z * z - 2.0 * (z * z) + z; };

  const Series value = nth_derivative(poly, x, 0);
  const Series first = nth_derivative(poly, x, 1);
  const Series second = nth_derivative(poly, x, 2);

  REQUIRE(value.item<double>() == Approx(0.078125));
  REQUIRE(first.item<double>() == Approx(0.6875));
  REQUIRE(second.item<double>() == Approx(3.5));
  REQUIRE_THROWS(nth_derivative(poly, x, 3));
}

TEST_CASE("vector parameter gradients preserve order and shape", "[autodiff][params]") {
  std::vector<Series> params{
      make_series(0.25).to(ScalarType::Float64),
      make_series(-0.1).to(ScalarType::Float64),
  };
  const Series x = pluss::linspace(-1.0, 1.0, 16, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  const auto [value, grads] = value_and_grad(
      [&](const std::vector<Trace>& p) -> Trace {
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

TEST_CASE("transform validation rejects invalid tangent shape", "[autodiff][validation]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series bad_tangent = make_series(std::vector<double>{1.0, -1.0}, {2}).to(ScalarType::Float64);
  REQUIRE_THROWS(jvp(
      [](const DualTrace& z) -> DualTrace { return vector_objective(z); },
      x,
      bad_tangent));
}

TEST_CASE("hvp validation rejects non-scalar objectives", "[autodiff][validation]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});
  REQUIRE_THROWS(hvp(
      [](const auto& z) { return vector_objective(z); },
      x,
      tangent));
}

TEST_CASE("hvp exact/fd mode policy is explicit and fail-loud", "[autodiff][hvp][policy]") {
  const Series x = make_f64({0.3, -0.2, 0.5});
  const Series tangent = make_f64({0.1, 0.05, -0.2});

  HvpOptions exact_opt{};
  exact_opt.mode = HvpOptions::Mode::ExactOnly;
  const auto [value_exact, hv_exact] = hvp(
      [](const auto& z) {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      tangent,
      exact_opt);
  REQUIRE(value_exact.numel() == 1);
  REQUIRE(hv_exact.numel() == 3);

  HvpOptions fd_only_opt{};
  fd_only_opt.mode = HvpOptions::Mode::FiniteDifferenceOnly;
  REQUIRE_THROWS(hvp(
      [](const DualTrace& z) -> DualTrace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      tangent,
      fd_only_opt));

  REQUIRE_THROWS(hvp(
      [](const Trace& z) -> Trace {
        using pluss::subsp::math::diff::sin;
        using pluss::subsp::math::diff::square;
        using pluss::subsp::math::diff::sum;
        return sum(sin(z) + square(z));
      },
      x,
      tangent,
      exact_opt));
}

TEST_CASE("reverse diagnostics include context on seeded reverse failures", "[autodiff][diagnostics]") {
  Series x = make_f64({0.3, -0.2, 0.5});
  Trace tx = trace(x, true);
  Trace y = tx * tx;
  const Series bad_seed = make_series(std::vector<double>{1.0, -1.0}, {2}).to(ScalarType::Float64);

  bool threw = false;
  std::string message;
  try {
    y.reverse(bad_seed);
  } catch (const std::exception& e) {
    threw = true;
    message = e.what();
  }
  REQUIRE(threw);
  REQUIRE(message.find("Trace::reverse") != std::string::npos);
  REQUIRE(message.find("hint") != std::string::npos);
}

TEST_CASE("autodiff remains finite under repeated nonlinear training steps", "[autodiff][stability]") {
  const Series x = pluss::linspace(-2.0, 2.0, 128, ScalarType::Float64);
  const Series y = x.mul(1.8).add(0.25).tanh();

  std::vector<Series> params{
      make_series(0.1).to(ScalarType::Float64),
      make_series(-0.2).to(ScalarType::Float64),
  };

  for (int i = 0; i < 120; ++i) {
    const auto [loss, grads] = value_and_grad(
        [&](const std::vector<Trace>& p) -> Trace {
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
