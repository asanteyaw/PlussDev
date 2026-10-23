#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace gas = pluss::subsp::natsci::physics::fluids::gas;

using pluss::ScalarType;
using pluss::Series;

namespace {

constexpr double pi = 3.141592653589793238462643383279502884;

void expect(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

common::ScalarQuantity<common::Length> metres(double value) {
  return common::ScalarQuantity<common::Length>::from_si(value);
}

spatial::BoxGrid box1(std::int64_t cells, double width) {
  return spatial::make_box_grid({cells}, {metres(width)});
}

spatial::BoxGrid box2(std::int64_t nx, std::int64_t ny, double dx,
                      double dy) {
  return spatial::make_box_grid({nx, ny}, {metres(dx), metres(dy)});
}

spatial::PeriodicGrid periodic2(std::int64_t cells, double width) {
  return spatial::make_periodic_grid({cells, cells},
                                     {metres(width), metres(width)});
}

template <class Grid>
gas::State primitive(const Series &rho, const Series &velocity,
                     const Series &pressure, const Grid &grid,
                     const gas::Model &model = {}) {
  const std::int64_t component = rho.dim() - grid.dim();
  const Series momentum = velocity.mul(rho.unsqueeze(component));
  const Series kinetic =
      velocity.square().sum(component, false).mul(rho).mul(0.5);
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho),
                   common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(
                       pressure.div(model.gamma - 1.0).add(kinetic)),
                   grid, model);
}

gas::Primitive primitive_data(double rho, double velocity, double pressure) {
  return gas::primitive(
      common::SeriesQuantity<gas::Density>::from_si(
          pluss::series::full({}, rho, ScalarType::Float64)),
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::make({velocity}, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::full({}, pressure, ScalarType::Float64)));
}

gas::Primitive primitive_data(double rho, double u, double v,
                              double pressure) {
  return gas::primitive(
      common::SeriesQuantity<gas::Density>::from_si(
          pluss::series::full({}, rho, ScalarType::Float64)),
      common::SeriesQuantity<common::Speed>::from_si(
          pluss::series::make({u, v}, ScalarType::Float64)),
      common::SeriesQuantity<common::Pressure>::from_si(
          pluss::series::full({}, pressure, ScalarType::Float64)));
}

gas::State final_state(const gas::FlowPath &path, const spatial::BoxGrid &grid,
                       const gas::Model &model = {}) {
  const auto &record = path.trajectory.state();
  return gas::make(
      common::SeriesQuantity<gas::Density>::from_si(
          record.get<gas::Density>("density").si_value().select(0, -1)),
      common::SeriesQuantity<gas::MomDensity>::from_si(
          record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
      common::SeriesQuantity<gas::EnergyDensity>::from_si(
          record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)),
      grid, model);
}

gas::State final_state(const gas::FlowPath &path,
                       const spatial::PeriodicGrid &grid,
                       const gas::Model &model = {}) {
  const auto &record = path.trajectory.state();
  return gas::make(
      common::SeriesQuantity<gas::Density>::from_si(
          record.get<gas::Density>("density").si_value().select(0, -1)),
      common::SeriesQuantity<gas::MomDensity>::from_si(
          record.get<gas::MomDensity>("momentum").si_value().select(0, -1)),
      common::SeriesQuantity<gas::EnergyDensity>::from_si(
          record.get<gas::EnergyDensity>("energy").si_value().select(0, -1)),
      grid, model);
}

struct PrimitiveState final {
  double rho;
  double velocity;
  double pressure;
};

struct PressureFunction final {
  double value;
  double derivative;
};

PressureFunction pressure_function(double pressure, const PrimitiveState &side,
                                   double gamma) {
  const double sound = std::sqrt(gamma * side.pressure / side.rho);
  if (pressure > side.pressure) {
    const double a = 2.0 / ((gamma + 1.0) * side.rho);
    const double b = (gamma - 1.0) * side.pressure / (gamma + 1.0);
    const double root = std::sqrt(a / (pressure + b));
    return {(pressure - side.pressure) * root,
            root * (1.0 - 0.5 * (pressure - side.pressure) /
                              (pressure + b))};
  }
  const double exponent = (gamma - 1.0) / (2.0 * gamma);
  const double ratio = pressure / side.pressure;
  return {2.0 * sound / (gamma - 1.0) * (std::pow(ratio, exponent) - 1.0),
          std::pow(ratio, -(gamma + 1.0) / (2.0 * gamma)) /
              (side.rho * sound)};
}

std::pair<double, double> star_state(const PrimitiveState &left,
                                     const PrimitiveState &right,
                                     double gamma) {
  const double left_sound = std::sqrt(gamma * left.pressure / left.rho);
  const double right_sound = std::sqrt(gamma * right.pressure / right.rho);
  double pressure = std::max(
      1e-14, 0.5 * (left.pressure + right.pressure) -
                 0.125 * (right.velocity - left.velocity) *
                     (left.rho + right.rho) * (left_sound + right_sound));
  for (int iteration = 0; iteration < 80; ++iteration) {
    const auto lf = pressure_function(pressure, left, gamma);
    const auto rf = pressure_function(pressure, right, gamma);
    const double next = std::max(
        1e-14, pressure -
                   (lf.value + rf.value + right.velocity - left.velocity) /
                       (lf.derivative + rf.derivative));
    if (std::abs(next - pressure) <=
        1e-13 * std::max(1.0, 0.5 * (next + pressure))) {
      pressure = next;
      break;
    }
    pressure = next;
  }
  const auto lf = pressure_function(pressure, left, gamma);
  const auto rf = pressure_function(pressure, right, gamma);
  return {pressure,
          0.5 * (left.velocity + right.velocity + rf.value - lf.value)};
}

PrimitiveState sample_riemann(double similarity, const PrimitiveState &left,
                              const PrimitiveState &right, double gamma) {
  const auto [star_pressure, star_velocity] =
      star_state(left, right, gamma);
  const double ratio = (gamma - 1.0) / (gamma + 1.0);
  if (similarity <= star_velocity) {
    const double sound = std::sqrt(gamma * left.pressure / left.rho);
    if (star_pressure > left.pressure) {
      const double shock =
          left.velocity -
          sound * std::sqrt((gamma + 1.0) * star_pressure /
                                (2.0 * gamma * left.pressure) +
                            (gamma - 1.0) / (2.0 * gamma));
      if (similarity <= shock)
        return left;
      return {left.rho *
                  ((star_pressure / left.pressure + ratio) /
                   (ratio * star_pressure / left.pressure + 1.0)),
              star_velocity, star_pressure};
    }
    const double star_sound =
        sound * std::pow(star_pressure / left.pressure,
                         (gamma - 1.0) / (2.0 * gamma));
    if (similarity <= left.velocity - sound)
      return left;
    if (similarity >= star_velocity - star_sound)
      return {left.rho *
                  std::pow(star_pressure / left.pressure, 1.0 / gamma),
              star_velocity, star_pressure};
    const double fan_sound =
        2.0 / (gamma + 1.0) *
        (sound + 0.5 * (gamma - 1.0) * (left.velocity - similarity));
    const double fan_velocity =
        2.0 / (gamma + 1.0) *
        (sound + 0.5 * (gamma - 1.0) * left.velocity + similarity);
    return {left.rho * std::pow(fan_sound / sound, 2.0 / (gamma - 1.0)),
            fan_velocity,
            left.pressure *
                std::pow(fan_sound / sound,
                         2.0 * gamma / (gamma - 1.0))};
  }

  const double sound = std::sqrt(gamma * right.pressure / right.rho);
  if (star_pressure > right.pressure) {
    const double shock =
        right.velocity +
        sound * std::sqrt((gamma + 1.0) * star_pressure /
                              (2.0 * gamma * right.pressure) +
                          (gamma - 1.0) / (2.0 * gamma));
    if (similarity >= shock)
      return right;
    return {right.rho *
                ((star_pressure / right.pressure + ratio) /
                 (ratio * star_pressure / right.pressure + 1.0)),
            star_velocity, star_pressure};
  }
  const double star_sound =
      sound * std::pow(star_pressure / right.pressure,
                       (gamma - 1.0) / (2.0 * gamma));
  if (similarity >= right.velocity + sound)
    return right;
  if (similarity <= star_velocity + star_sound)
    return {right.rho * std::pow(star_pressure / right.pressure, 1.0 / gamma),
            star_velocity, star_pressure};
  const double fan_sound =
      2.0 / (gamma + 1.0) *
      (sound - 0.5 * (gamma - 1.0) * (right.velocity - similarity));
  const double fan_velocity =
      2.0 / (gamma + 1.0) *
      (-sound + 0.5 * (gamma - 1.0) * right.velocity + similarity);
  return {right.rho * std::pow(fan_sound / sound, 2.0 / (gamma - 1.0)),
          fan_velocity,
          right.pressure *
              std::pow(fan_sound / sound, 2.0 * gamma / (gamma - 1.0))};
}

void test_lax_riemann_problem() {
  constexpr std::int64_t cells = 320;
  constexpr double stop = 0.14;
  constexpr PrimitiveState left{0.445, 0.698, 3.528};
  constexpr PrimitiveState right{0.5, 0.0, 0.571};
  const auto grid = box1(cells, 1.0 / cells);
  const Series x = pluss::series::arange(0, cells, 1, ScalarType::Float64)
                       .add(0.5)
                       .div(static_cast<double>(cells));
  const Series mask = x.lt(0.5);
  const Series rho = mask.where(pluss::series::full_like(x, left.rho),
                                pluss::series::full_like(x, right.rho));
  const Series velocity =
      mask.where(pluss::series::full_like(x, left.velocity),
                 pluss::series::full_like(x, right.velocity));
  const Series pressure =
      mask.where(pluss::series::full_like(x, left.pressure),
                 pluss::series::full_like(x, right.pressure));
  const auto initial = primitive(rho, velocity.unsqueeze(0), pressure, grid);
  const auto rules = gas::bounds(
      grid,
      {gas::fixed({0, spatial::Side::Low},
                  primitive_data(left.rho, left.velocity, left.pressure)),
       gas::fixed({0, spatial::Side::High},
                  primitive_data(right.rho, right.velocity, right.pressure))});
  gas::RunOptions options;
  options.safety = 0.92;
  options.step.rule = gas::Rule::Rk3;
  options.step.recon = gas::Recon::Weno5;
  options.step.flux = gas::FluxScheme::Hllc;
  const auto path = gas::evolve_to(initial, grid, rules, seconds(0.0),
                                   seconds(stop), {}, options);
  const auto result = final_state(path, grid);
  const auto density = result.density().si_value().to_vec<double>();
  const auto speed =
      gas::velocity(result, grid).si_value().select(0, 0).to_vec<double>();
  const auto p = gas::pressure(result, grid).si_value().to_vec<double>();
  double density_l1 = 0.0;
  double velocity_l1 = 0.0;
  double pressure_l1 = 0.0;
  for (std::int64_t index = 0; index < cells; ++index) {
    const double coordinate = (static_cast<double>(index) + 0.5) / cells;
    const auto exact = sample_riemann((coordinate - 0.5) / stop, left, right,
                                      1.4);
    density_l1 += std::abs(density[static_cast<std::size_t>(index)] - exact.rho);
    velocity_l1 +=
        std::abs(speed[static_cast<std::size_t>(index)] - exact.velocity);
    pressure_l1 +=
        std::abs(p[static_cast<std::size_t>(index)] - exact.pressure);
  }
  density_l1 /= cells;
  velocity_l1 /= cells;
  pressure_l1 /= cells;
  expect(density_l1 < 0.018 && velocity_l1 < 0.026 && pressure_l1 < 0.035,
         "Lax solution exceeded the exact-Riemann L1 error envelope: rho=" +
             std::to_string(density_l1) + ", u=" +
             std::to_string(velocity_l1) + ", p=" +
             std::to_string(pressure_l1));
}

void test_shu_osher_wave_retention() {
  constexpr std::int64_t cells = 400;
  constexpr double low = -5.0;
  constexpr double high = 5.0;
  constexpr double stop = 1.8;
  const double width = (high - low) / cells;
  const auto grid = box1(cells, width);
  const Series x = pluss::series::arange(0, cells, 1, ScalarType::Float64)
                       .add(0.5)
                       .mul(width)
                       .add(low);
  const Series mask = x.lt(-4.0);
  const Series rho = mask.where(
      pluss::series::full_like(x, 3.857143),
      x.mul(5.0).sin().mul(0.2).add(1.0));
  const Series velocity = mask.where(pluss::series::full_like(x, 2.629369),
                                     pluss::series::zeros_like(x));
  const Series pressure = mask.where(pluss::series::full_like(x, 10.33333),
                                     pluss::series::ones_like(x));
  const auto initial = primitive(rho, velocity.unsqueeze(0), pressure, grid);
  const auto rules = gas::bounds(
      grid,
      {gas::fixed({0, spatial::Side::Low},
                  primitive_data(3.857143, 2.629369, 10.33333)),
       gas::outlet({0, spatial::Side::High})});
  gas::RunOptions options;
  options.safety = 0.9;
  options.step.rule = gas::Rule::Rk3;
  options.step.recon = gas::Recon::Weno5;
  options.step.flux = gas::FluxScheme::Hllc;
  const auto path = gas::evolve_to(initial, grid, rules, seconds(0.0),
                                   seconds(stop), {}, options);
  const auto result = final_state(path, grid);
  const Series end_rho = result.density().si_value();
  const Series end_pressure = gas::pressure(result, grid).si_value();
  expect(end_rho.gt(0.0).logical_and(end_rho.isfinite()).all().item<bool>() &&
             end_pressure.gt(0.0)
                 .logical_and(end_pressure.isfinite())
                 .all()
                 .item<bool>(),
         "Shu-Osher evolution lost gas admissibility");

  // At t=1.8 the Mach-3 shock is near x=2.4. The interval [-1.5, 1.5]
  // contains the entropy waves transmitted behind it without including the
  // shock itself. A first-order reconstruction provides a dissipation oracle.
  const Series wave_region = x.ge(-1.5).logical_and(x.le(1.5));
  const double resolved_variation =
      end_rho.masked_select(wave_region).std(false).item<double>();
  auto diffuse_options = options;
  diffuse_options.step.rule = gas::Rule::Rk2;
  diffuse_options.step.recon = gas::Recon::Cell;
  diffuse_options.step.flux = gas::FluxScheme::Rusanov;
  const auto diffuse = final_state(
      gas::evolve_to(initial, grid, rules, seconds(0.0), seconds(stop), {},
                     diffuse_options),
      grid);
  const double diffuse_variation =
      diffuse.density()
          .si_value()
          .masked_select(wave_region)
          .std(false)
          .item<double>();
  expect(resolved_variation > 0.12 &&
             resolved_variation > 1.35 * diffuse_variation,
         "Shu-Osher WENO path did not retain the post-shock entropy waves: "
         "resolved=" +
             std::to_string(resolved_variation) + ", first-order=" +
             std::to_string(diffuse_variation));
}

struct Vortex final {
  Series density;
  Series velocity;
  Series pressure;
};

Vortex vortex(std::int64_t cells, double time) {
  constexpr double length = 10.0;
  constexpr double gamma = 1.4;
  constexpr double beta = 2.0;
  const double width = length / cells;
  Series x = pluss::series::arange(0, cells, 1, ScalarType::Float64)
                 .add(0.5)
                 .mul(width)
                 .sub(time);
  Series y = x;
  // Periodic minimum-image displacement from the translated centre.
  x = x.sub(5.0).add(0.5 * length).fmod(length).add(length).fmod(length).sub(
      0.5 * length);
  y = y.sub(5.0).add(0.5 * length).fmod(length).add(length).fmod(length).sub(
      0.5 * length);
  const Series dx = x.reshape({cells, 1});
  const Series dy = y.reshape({1, cells});
  const Series radius2 = dx.square().add(dy.square());
  const Series envelope = radius2.neg().add(1.0).mul(0.5).exp();
  const Series temperature =
      radius2.neg()
          .add(1.0)
          .exp()
          .mul(-(gamma - 1.0) * beta * beta /
               (8.0 * gamma * pi * pi))
          .add(1.0);
  const Series density = temperature.pow(1.0 / (gamma - 1.0));
  const Series u = dy.mul(envelope).mul(-beta / (2.0 * pi)).add(1.0);
  const Series v = dx.mul(envelope).mul(beta / (2.0 * pi)).add(1.0);
  return {density, u.unsqueeze(0).unite({v.unsqueeze(0)}, 0),
          density.pow(gamma)};
}

double vortex_error(std::int64_t cells, double stop) {
  constexpr double length = 10.0;
  const auto grid = periodic2(cells, length / cells);
  const auto initial_data = vortex(cells, 0.0);
  const auto initial = primitive(initial_data.density, initial_data.velocity,
                                 initial_data.pressure, grid);
  gas::RunOptions options;
  options.safety = 0.88;
  options.step.rule = gas::Rule::Rk3;
  options.step.recon = gas::Recon::Weno5;
  options.step.flux = gas::FluxScheme::Hllc;
  const auto result = final_state(gas::evolve_to(initial, grid, seconds(0.0),
                                                 seconds(stop), {}, options),
                                  grid);
  const Series exact = vortex(cells, stop).density;
  return result.density().si_value().sub(exact).abs().mean().item<double>();
}

void test_isentropic_vortex_convergence() {
  constexpr double stop = 0.5;
  const double coarse = vortex_error(24, stop);
  const double fine = vortex_error(48, stop);
  const double order = std::log(coarse / fine) / std::log(2.0);
  expect(fine < 2.5e-4 && order > 1.8,
         "isentropic-vortex density did not converge against its translated "
         "analytic solution: coarse=" +
             std::to_string(coarse) + ", fine=" + std::to_string(fine) +
             ", order=" + std::to_string(order));
}

double transverse_density_defect(const gas::State &state) {
  const Series rho = state.density().si_value();
  return rho.sub(rho.mean(1, true)).square().mean().sqrt().item<double>();
}

void test_quirk_cartesian_carbuncle() {
  // Cartesian analogue of Quirk's perturbed-grid experiment: a Mach-6 shock
  // crosses a duct whose pre-shock state carries a deterministic grid-scale
  // perturbation. The exact solution is one-dimensional, so transverse
  // density variance directly measures odd-even shock-front growth.
  constexpr std::int64_t nx = 120;
  constexpr std::int64_t ny = 12;
  constexpr double spacing = 0.1;
  constexpr double stop = 0.6;
  const auto grid = box2(nx, ny, spacing, spacing);
  const Series ix =
      pluss::series::arange(0, nx, 1, ScalarType::Float64).reshape({nx, 1});
  const Series iy =
      pluss::series::arange(0, ny, 1, ScalarType::Float64).reshape({1, ny});
  const Series post = ix.lt(5.0);
  const Series sign = ix.add(iy).fmod(2.0).eq(0.0).where(
      pluss::series::ones({nx, ny}, ScalarType::Float64),
      pluss::series::full({nx, ny}, -1.0, ScalarType::Float64));
  const Series noise = post.logical_not().where(sign.mul(1e-3),
                                                 pluss::series::zeros_like(sign));
  const Series rho = post.where(pluss::series::full_like(sign, 5.2682926829),
                                noise.add(1.0));
  const Series u = post.where(pluss::series::full_like(sign, 5.751744607),
                              noise.mul(0.1));
  const Series v = noise.mul(0.1);
  const Series pressure =
      post.where(pluss::series::full_like(sign, 41.8333333333),
                 noise.mul(0.1).add(1.0));
  const auto initial = primitive(
      rho, u.unsqueeze(0).unite({v.unsqueeze(0)}, 0), pressure, grid);
  const auto rules = gas::bounds(
      grid,
      {gas::fixed({0, spatial::Side::Low},
                  gas::primitive(
                      common::SeriesQuantity<gas::Density>::from_si(
                          pluss::series::full({}, 5.2682926829,
                                              ScalarType::Float64)),
                      common::SeriesQuantity<common::Speed>::from_si(
                          pluss::series::make({5.751744607, 0.0},
                                              ScalarType::Float64)),
                      common::SeriesQuantity<common::Pressure>::from_si(
                          pluss::series::full({}, 41.8333333333,
                                              ScalarType::Float64)))),
       gas::outlet({0, spatial::Side::High}),
       gas::slip({1, spatial::Side::Low}),
       gas::slip({1, spatial::Side::High})});
  gas::RunOptions classic;
  classic.safety = 0.9;
  classic.step.rule = gas::Rule::Rk2;
  classic.step.recon = gas::Recon::Cell;
  classic.step.flux = gas::FluxScheme::Roe;
  classic.step.roe.entropy = 0.05;
  const auto uncorrected = final_state(
      gas::evolve_to(initial, grid, rules, seconds(0.0), seconds(stop), {},
                     classic),
      grid);
  auto corrected_options = classic;
  corrected_options.step.roe.shock = gas::RoeShock::HFix;
  const auto corrected = final_state(
      gas::evolve_to(initial, grid, rules, seconds(0.0), seconds(stop), {},
                     corrected_options),
      grid);
  const double classic_defect = transverse_density_defect(uncorrected);
  const double corrected_defect = transverse_density_defect(corrected);
  expect(corrected_defect < 0.65 * classic_defect,
         "Roe H-correction did not suppress Quirk odd-even growth: classic=" +
             std::to_string(classic_defect) + ", corrected=" +
             std::to_string(corrected_defect));
}

void test_double_mach_reflection() {
  constexpr std::int64_t nx = 120;
  constexpr std::int64_t ny = 30;
  constexpr double x_length = 4.0;
  constexpr double y_length = 1.0;
  constexpr double stop = 0.2;
  constexpr double root3 = 1.7320508075688772935;
  constexpr double sin60 = 0.86602540378443864676;
  constexpr double shock_foot = 1.0 / 6.0;
  constexpr double post_rho = 8.0;
  constexpr double post_u = 7.144709581221618;
  constexpr double post_v = -4.125;
  constexpr double post_pressure = 116.5;
  constexpr double pre_rho = 1.4;
  constexpr double pre_pressure = 1.0;
  const double dx = x_length / nx;
  const double dy = y_length / ny;
  const auto grid = box2(nx, ny, dx, dy);
  const Series x_lane =
      pluss::series::arange(0, nx, 1, ScalarType::Float64).add(0.5).mul(dx);
  const Series y_lane =
      pluss::series::arange(0, ny, 1, ScalarType::Float64).add(0.5).mul(dy);
  const Series x = x_lane.reshape({nx, 1});
  const Series y = y_lane.reshape({1, ny});
  const Series post = x.lt(y.div(root3).add(shock_foot));
  const Series field = x.add(y);
  const Series rho = post.where(pluss::series::full_like(field, post_rho),
                                pluss::series::full_like(field, pre_rho));
  const Series u = post.where(pluss::series::full_like(rho, post_u),
                              pluss::series::zeros_like(rho));
  const Series v = post.where(pluss::series::full_like(rho, post_v),
                              pluss::series::zeros_like(rho));
  const Series pressure =
      post.where(pluss::series::full_like(rho, post_pressure),
                 pluss::series::full_like(rho, pre_pressure));
  const auto initial = primitive(
      rho, u.unsqueeze(0).unite({v.unsqueeze(0)}, 0), pressure, grid);
  const auto post_state =
      primitive_data(post_rho, post_u, post_v, post_pressure);
  std::int64_t boundary_calls = 0;
  double latest_stage = 0.0;
  gas::BoundFn moving = [&](common::ScalarQuantity<common::Time> time) {
    ++boundary_calls;
    latest_stage = std::max(latest_stage, time.si_value());
    const double top_crossing =
        shock_foot + 1.0 / root3 + 10.0 * time.si_value() / sin60;
    const Series top_post = x_lane.lt(top_crossing);
    const Series top_rho = top_post.where(
        pluss::series::full_like(x_lane, post_rho),
        pluss::series::full_like(x_lane, pre_rho));
    const Series top_u = top_post.where(
        pluss::series::full_like(x_lane, post_u),
        pluss::series::zeros_like(x_lane));
    const Series top_v = top_post.where(
        pluss::series::full_like(x_lane, post_v),
        pluss::series::zeros_like(x_lane));
    const Series top_pressure = top_post.where(
        pluss::series::full_like(x_lane, post_pressure),
        pluss::series::full_like(x_lane, pre_pressure));
    const auto top_state = gas::primitive(
        common::SeriesQuantity<gas::Density>::from_si(top_rho),
        common::SeriesQuantity<common::Speed>::from_si(
            top_u.unsqueeze(0).unite({top_v.unsqueeze(0)}, 0)),
        common::SeriesQuantity<common::Pressure>::from_si(top_pressure));
    const Series fixed_floor = x_lane.lt(shock_foot);
    return gas::bounds(
        grid,
        {gas::fixed({0, spatial::Side::Low}, post_state),
         gas::outlet({0, spatial::Side::High}),
         gas::blend(fixed_floor,
                    gas::fixed({1, spatial::Side::Low}, post_state),
                    gas::slip({1, spatial::Side::Low})),
         gas::fixed({1, spatial::Side::High}, top_state)});
  };
  gas::RunOptions options;
  options.safety = 0.82;
  options.step.rule = gas::Rule::Rk3;
  options.step.recon = gas::Recon::Weno5;
  options.step.flux = gas::FluxScheme::Roe;
  options.step.roe.shock = gas::RoeShock::HFix;
  const auto path = gas::evolve_to(initial, grid, moving, seconds(0.0),
                                   seconds(stop), {}, options);
  const auto result = final_state(path, grid);
  const Series result_rho = result.density().si_value();
  const Series result_pressure = gas::pressure(result, grid).si_value();
  expect(result_rho.gt(0.0)
                 .logical_and(result_rho.isfinite())
                 .logical_and(result_pressure.gt(0.0))
                 .logical_and(result_pressure.isfinite())
                 .all()
                 .item<bool>(),
         "double-Mach reflection lost gas admissibility");
  expect(boundary_calls >= 3 * path.steps &&
             latest_stage >= stop - 2e-12 && latest_stage <= stop + 2e-12,
         "double-Mach moving boundary was not evaluated through the final "
         "SSP-RK stage");

  const double top_crossing =
      shock_foot + 1.0 / root3 + 10.0 * stop / sin60;
  const Series expected_top = x_lane.lt(top_crossing);
  const double top_agreement =
      result_rho.select(1, ny - 1)
          .gt(4.0)
          .eq(expected_top)
          .to(ScalarType::Float64)
          .mean()
          .item<double>();
  const auto bottom = result_rho.select(1, 0).to_vec<double>();
  double mach_stem_front = 0.0;
  for (std::int64_t index = 0; index < nx; ++index)
    if (bottom[static_cast<std::size_t>(index)] > 4.0)
      mach_stem_front = (static_cast<double>(index) + 0.5) * dx;
  expect(top_agreement > 0.94 && mach_stem_front > 1.5 &&
             result_rho.max().item<double>() > post_rho,
         "double-Mach reflection did not form the prescribed moving shock and "
         "reflected Mach stem: top agreement=" +
             std::to_string(top_agreement) + ", stem=" +
             std::to_string(mach_stem_front) + ", rho_max=" +
             std::to_string(result_rho.max().item<double>()));
}

} // namespace

int main() {
  try {
    test_lax_riemann_problem();
    test_shu_osher_wave_retention();
    test_isentropic_vortex_convergence();
    test_quirk_cartesian_carbuncle();
    test_double_mach_reflection();
    std::cout << "NatSci compressible benchmark checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
