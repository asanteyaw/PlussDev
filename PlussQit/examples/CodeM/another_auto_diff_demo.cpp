#include <pluss/pluss.h>

#include <cmath>
#include <functional>
#include <iostream>
#include <string_view>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::subsp::math::diff::nth_diff;
using pluss::subsp::math::diff::trace;

template <class Fn>
bool report_case(std::string_view name,
                 double x0,
                 Fn&& fn,
                 double expected_value,
                 double expected_first,
                 double expected_second,
                 double tol = 1e-9) {
  try {
    const Series x = pluss::make_series(x0).to(ScalarType::Float64);
    auto tracked_x = trace(x, true);
    auto tracked_y = fn(tracked_x);
    tracked_y.reverse();

    const Series value = tracked_y.value();
    const Series first = tracked_x.sensitivity();
    const Series second = nth_diff(
        [&](const auto& z) { return fn(z); },
        x,
        2);

    const double value_err = std::abs(value.template item<double>() - expected_value);
    const double first_err = std::abs(first.template item<double>() - expected_first);
    const double second_err = std::abs(second.template item<double>() - expected_second);

    std::cout << name
              << " x=" << x0
              << " value=" << value.template item<double>()
              << " reverse_grad=" << first.template item<double>()
              << " nth_second=" << second.template item<double>()
              << " errs=(" << value_err << "," << first_err << "," << second_err << ")\n";

    return value_err <= tol && first_err <= tol && second_err <= tol;
  } catch (const std::exception& ex) {
    std::cerr << name << " failed: " << ex.what() << '\n';
    return false;
  }
}

} // namespace

int main() {
  using namespace pluss::subsp::math::diff;

  bool ok = true;

  ok &= report_case(
      "linear",
      1.25,
      [](const auto& x) { return 3.0 * x + 2.0; },
      3.0 * 1.25 + 2.0,
      3.0,
      0.0);

  ok &= report_case(
      "quadratic",
      1.25,
      [](const auto& x) { return x * x - 4.0 * x + 1.0; },
      1.25 * 1.25 - 4.0 * 1.25 + 1.0,
      2.0 * 1.25 - 4.0,
      2.0);

  ok &= report_case(
      "polynomial",
      1.25,
      [](const auto& x) { return x * x * x - 2.0 * (x * x) + x; },
      std::pow(1.25, 3.0) - 2.0 * std::pow(1.25, 2.0) + 1.25,
      3.0 * std::pow(1.25, 2.0) - 4.0 * 1.25 + 1.0,
      6.0 * 1.25 - 4.0);

  ok &= report_case(
      "transcendental",
      1.5,
      [](const auto& x) { return exp(x) + log(x); },
      std::exp(1.5) + std::log(1.5),
      std::exp(1.5) + 1.0 / 1.5,
      std::exp(1.5) - 1.0 / (1.5 * 1.5));

  ok &= report_case(
      "trigonometric",
      0.4,
      [](const auto& x) { return sin(x) + cos(x); },
      std::sin(0.4) + std::cos(0.4),
      std::cos(0.4) - std::sin(0.4),
      -std::sin(0.4) - std::cos(0.4));

  ok &= report_case(
      "hyperbolic",
      0.4,
      [](const auto& x) { return tanh(x); },
      std::tanh(0.4),
      1.0 - std::pow(std::tanh(0.4), 2.0),
      -2.0 * std::tanh(0.4) * (1.0 - std::pow(std::tanh(0.4), 2.0)),
      1e-8);

  ok &= report_case(
      "softplus",
      0.4,
      [](const auto& x) { return log(1.0 + exp(x)); },
      std::log1p(std::exp(0.4)),
      1.0 / (1.0 + std::exp(-0.4)),
      (1.0 / (1.0 + std::exp(-0.4))) * (1.0 - 1.0 / (1.0 + std::exp(-0.4))),
      1e-8);

  if (!ok) {
    std::cerr << "autodiff derivatives demo failed\n";
    return 1;
  }

  return 0;
}
