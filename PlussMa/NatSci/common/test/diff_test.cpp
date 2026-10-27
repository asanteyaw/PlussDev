#include "pluss/codem/series_factory.h"
#include "pluss/ma/natsci/common.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace common = pluss::ma::natsci::common;
namespace diff = common::diff;

namespace {

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool close(double actual, double expected, double relative = 1e-9) {
  const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
  return std::abs(actual - expected) <= relative * scale;
}

void expect_shape(const pluss::Series &value,
                  std::initializer_list<std::int64_t> expected,
                  const std::string &message) {
  expect(value.dim() == static_cast<std::int64_t>(expected.size()), message);
  std::size_t axis = 0;
  for (const auto extent : expected)
    expect(value.size(static_cast<std::int64_t>(axis++)) == extent, message);
}

auto squared_length = [](const common::SeriesQuantity<common::Length> &x) {
  return common::SeriesQuantity<common::Area>::from_si(x.si_value().square());
};

auto summed_squared_length =
    [](const common::SeriesQuantity<common::Length> &x) {
      return common::SeriesQuantity<common::Area>::from_si(
          x.si_value().square().sum());
    };

void test_gradient_and_jacobian() {
  const auto x = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::make({2.0, 3.0}));
  const auto gradient =
      diff::grad<common::Area, common::Length>(summed_squared_length, x);
  static_assert(
      std::same_as<typename decltype(gradient)::dimension, common::Length>);
  expect(gradient.si_value().allclose(pluss::series::make({4.0, 6.0}), 1e-12,
                                      1e-12),
         "dimension-aware gradient is incorrect");

  pluss::subsp::math::diff::JacobianOptions options;
  options.vectorized = true;
  const auto [value, jacobian] =
      diff::jacobian<common::Area, common::Length>(squared_length, x, options);
  expect(value.si_value().equal(pluss::series::make({4.0, 9.0})),
         "Jacobian evaluation changed the primal value");
  expect_shape(jacobian.si_value(), {2, 2},
               "Jacobian did not expose output-by-input lanes");
  expect(jacobian.si_value().allclose(
             pluss::series::make({{4.0, 0.0}, {0.0, 6.0}}), 1e-12, 1e-12),
         "vectorized Jacobian is incorrect");
}

void test_hessian_and_hvp() {
  const auto x = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::make({2.0, 3.0}));
  pluss::subsp::math::diff::HessianOptions options;
  options.vectorized = true;
  const auto [value, hessian] = diff::hessian<common::Area, common::Length>(
      summed_squared_length, x, options);
  static_assert(std::same_as<typename decltype(hessian)::dimension,
                             common::Dimensionless>);
  expect(close(value.si_value().item<double>(), 13.0),
         "Hessian evaluation changed the primal value");
  expect_shape(hessian.si_value(), {2, 2},
               "Hessian did not preserve both input axes");
  expect(hessian.si_value().allclose(
             pluss::series::make({{2.0, 0.0}, {0.0, 2.0}}), 1e-8, 1e-8),
         "Hessian is incorrect");

  const auto direction = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::make({3.0, -4.0}));
  const auto [hvp_value, product] = diff::hvp<common::Area, common::Length>(
      summed_squared_length, x, direction);
  static_assert(
      std::same_as<typename decltype(product)::dimension, common::Length>);
  expect(close(hvp_value.si_value().item<double>(), 13.0),
         "HVP evaluation changed the primal value");
  expect(
      product.si_value().allclose(pluss::series::make({6.0, -8.0}), 1e-7, 1e-7),
      "Hessian-vector product is incorrect");
}

void test_higher_order_and_elasticity() {
  const auto x =
      common::SeriesQuantity<common::Length>::from_si(pluss::series::make(2.0));
  const auto fifth_power =
      [](const common::SeriesQuantity<common::Length> &value) {
        return common::SeriesQuantity<
            common::DimensionPower<common::Length, 5>>::from_si(value.si_value()
                                                                    .pow(5.0));
      };
  using FifthLength = common::DimensionPower<common::Length, 5>;
  const auto primal =
      diff::nth_diff<0, FifthLength, common::Length>(fifth_power, x);
  const auto first =
      diff::nth_diff<1, FifthLength, common::Length>(fifth_power, x);
  const auto second =
      diff::nth_diff<2, FifthLength, common::Length>(fifth_power, x);
  const auto third =
      diff::nth_diff<3, FifthLength, common::Length>(fifth_power, x);
  static_assert(
      std::same_as<typename decltype(primal)::dimension, FifthLength>);
  static_assert(std::same_as<typename decltype(first)::dimension,
                             common::DimensionPower<common::Length, 4>>);
  static_assert(std::same_as<typename decltype(second)::dimension,
                             common::DimensionPower<common::Length, 3>>);
  static_assert(std::same_as<typename decltype(third)::dimension,
                             common::DimensionPower<common::Length, 2>>);
  expect(close(primal.si_value().item<double>(), 32.0),
         "zeroth derivative did not return the objective");
  expect(close(first.si_value().item<double>(), 80.0),
         "first derivative is incorrect");
  expect(close(second.si_value().item<double>(), 160.0, 1e-8),
         "second derivative is incorrect");
  expect(close(third.si_value().item<double>(), 240.0, 1e-8),
         "third derivative is incorrect");

  const auto vector_x = common::SeriesQuantity<common::Length>::from_si(
      pluss::series::make({2.0, 3.0}));
  const auto component_second =
      diff::nth_diff<2, common::Area, common::Length>(squared_length, vector_x);
  static_assert(std::same_as<typename decltype(component_second)::dimension,
                             common::Dimensionless>);
  expect_shape(component_second.si_value(), {2, 2, 2},
               "second derivative lost output or repeated input axes");
  expect(close(component_second.si_value()
                   .select(0, 0)
                   .select(0, 0)
                   .select(0, 0)
                   .item<double>(),
               2.0),
         "component second derivative is incorrect");
  expect(close(component_second.si_value()
                   .select(0, 1)
                   .select(0, 1)
                   .select(0, 1)
                   .item<double>(),
               2.0),
         "component second derivative lost the second output lane");

  const auto cube = [](const common::SeriesQuantity<common::Length> &value) {
    return common::SeriesQuantity<common::Volume>::from_si(
        value.si_value().pow(3.0));
  };
  const auto normalized =
      diff::elasticity<common::Volume, common::Length>(cube, x);
  expect(close(normalized.si_value().item<double>(), 3.0),
         "elasticity is not x/y times dy/dx");

  const auto components = diff::elasticity<common::Area, common::Length>(
      summed_squared_length, vector_x);
  expect(components.si_value().allclose(
             pluss::series::make({8.0 / 13.0, 18.0 / 13.0}), 1e-12, 1e-12),
         "vector elasticity lost component contributions");
}

void test_nonleaf_transform_input() {
  pluss::Series base = pluss::series::make({2.0});
  base.tracks_slope_(true);
  const pluss::Series nonleaf = base.mul(3.0);

  pluss::subsp::math::diff::DerivativePolicy policy;
  policy.keep_graph = true;
  policy.track_higher_order = true;
  const auto [value, gradient] = pluss::subsp::math::diff::value_and_grad(
      [](const pluss::Series&x) {
        return x.square().sum();
      },
      nonleaf, policy);
  expect(close(value.item<double>(), 36.0),
         "non-leaf transform changed the primal value");
  expect(gradient.allclose(pluss::series::make({12.0}), 1e-12, 1e-12),
         "value_and_grad discarded a non-leaf input sensitivity");

  base.clear();
  gradient.sum().reverse();
  expect(
      base.has_sensitivity() &&
          base.sensitivity().allclose(pluss::series::make({6.0}), 1e-12, 1e-12),
      "non-leaf value_and_grad severed its outer derivative graph");
}

void test_strided_and_failures() {
  const auto base =
      pluss::series::make(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
  const auto strided = base.transpose(0, 1);
  expect(!strided.is_contiguous(), "test input must be non-contiguous");
  const auto x = common::SeriesQuantity<common::Length>::from_si(strided);
  const auto gradient =
      diff::grad<common::Area, common::Length>(summed_squared_length, x);
  expect_shape(gradient.si_value(), {3, 2},
               "gradient changed a non-contiguous input shape");
  expect(gradient.si_value().allclose(strided.mul(2.0), 1e-12, 1e-12),
         "gradient changed non-contiguous lane correspondence");

  bool vector_grad_rejected = false;
  try {
    (void)diff::grad<common::Area, common::Length>(squared_length, x);
  } catch (const std::invalid_argument &) {
    vector_grad_rejected = true;
  }
  expect(vector_grad_rejected,
         "gradient silently reduced a vector-valued callback");

  const auto full_third =
      diff::nth_diff<3, common::Area, common::Length>(squared_length, x);
  expect_shape(full_third.si_value(), {3, 2, 3, 2, 3, 2, 3, 2},
               "higher derivative did not preserve non-contiguous input axes");
  expect(full_third.si_value().eq(0.0).all().item<bool>(),
         "quadratic third derivative should be exactly zero");

  bool forward_mode_rejected = false;
  try {
    pluss::subsp::math::diff::JacobianOptions options;
    options.mode = pluss::subsp::math::diff::JacobianMode::Forward;
    (void)diff::jacobian<common::Area, common::Length>(squared_length, x,
                                                       options);
  } catch (const std::invalid_argument &) {
    forward_mode_rejected = true;
  }
  expect(forward_mode_rejected,
         "quantity Jacobian silently accepted unsupported forward mode");

  bool zero_elasticity_rejected = false;
  try {
    const auto zero = common::SeriesQuantity<common::Length>::from_si(
        pluss::series::make(0.0));
    (void)diff::elasticity<common::Area, common::Length>(squared_length, zero);
  } catch (const std::domain_error &) {
    zero_elasticity_rejected = true;
  }
  expect(zero_elasticity_rejected,
         "elasticity accepted a zero output denominator");
}

} // namespace

int main() {
  try {
    test_gradient_and_jacobian();
    test_hessian_and_hvp();
    test_higher_order_and_elasticity();
    test_nonleaf_transform_input();
    test_strided_and_failures();
    std::cout << "NatSci dimension-aware differentiation checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
