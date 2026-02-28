#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::diff::DualTrace;
using pluss::subsp::math::diff::HvpOptions;
using pluss::subsp::math::diff::Trace;
using pluss::subsp::math::diff::grad;
using pluss::subsp::math::diff::hvp;
using pluss::subsp::math::diff::jacfwd;
using pluss::subsp::math::diff::jacrev;
using pluss::subsp::math::diff::jvp;
using pluss::subsp::math::diff::linearize;
using pluss::subsp::math::diff::nth_derivative;
using pluss::subsp::math::diff::trace;
using pluss::subsp::math::diff::value_and_grad;
using pluss::subsp::math::diff::vjp;
using pluss::subsp::math::optim::ParameterPack;
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
  x.requires_grad_(true);

  Series loss = x.square().sum();
  loss.reverse();

  REQUIRE(x.has_sensitivity());
  REQUIRE(x.sensitivity().item<double>() == Approx(2.5));
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

TEST_CASE("structured parameter packs preserve names and gradients", "[autodiff][params]") {
  ParameterPack<Series> params({
      {"w", make_series(0.25).to(ScalarType::Float64)},
      {"b", make_series(-0.1).to(ScalarType::Float64)},
  });

  const Series x = pluss::linspace(-1.0, 1.0, 16, ScalarType::Float64);
  const Series y = x.mul(2.0).sub(0.5);

  const auto [value, grads] = pluss::subsp::math::optim::value_and_grad(
      [&](const ParameterPack<Trace>& p) -> Trace {
        const auto pred = p.at("w") * trace(x, false) + p.at("b");
        const auto err = pred - trace(y, false);
        return mean(err * err);
      },
      params);

  REQUIRE(value.numel() == 1);
  REQUIRE(grads.contains("w"));
  REQUIRE(grads.contains("b"));
  REQUIRE(grads.size() == 2);
  REQUIRE(grads.at("w").numel() == 1);
  REQUIRE(grads.at("b").numel() == 1);

  const auto layout = params.layout();
  REQUIRE(layout.size() == 2);
  REQUIRE(layout.total_numel() == 2);
  REQUIRE(layout.contains("w"));
  REQUIRE(layout.at("w").numel == 1);

  const Series flat = params.flatten_values();
  REQUIRE(flat.numel() == 2);

  const auto rebuilt = pluss::subsp::math::optim::unflatten_values(layout, flat);
  REQUIRE(rebuilt.same_layout(params));
  REQUIRE(rebuilt.at("w").item<double>() == Approx(params.at("w").item<double>()));
  REQUIRE(rebuilt.at("b").item<double>() == Approx(params.at("b").item<double>()));

  const auto zeros = params.zeros_like();
  REQUIRE(params.same_layout(zeros));
  REQUIRE(zeros.at("w").item<double>() == Approx(0.0));
  REQUIRE(zeros.at("b").item<double>() == Approx(0.0));
}
