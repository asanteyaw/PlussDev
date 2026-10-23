#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace power = pluss::eng::power;

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-9) {
  const auto actual = value.to_vec<double>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t i = 0; i < actual.size(); ++i) {
    if (std::abs(actual[i] - expected[i]) > tolerance)
      throw std::runtime_error("real result mismatch at lane " +
                               std::to_string(i) + ": got " +
                               std::to_string(actual[i]) + ", expected " +
                               std::to_string(expected[i]));
  }
}

void close_complex(const pluss::Series &value,
                   const std::vector<std::complex<double>> &expected,
                   double tolerance = 1e-9) {
  const auto actual =
      value.to(pluss::ScalarType::Complex128).to_vec<std::complex<double>>();
  expect(actual.size() == expected.size(), "complex result size mismatch");
  for (std::size_t i = 0; i < actual.size(); ++i)
    expect(std::abs(actual[i] - expected[i]) <= tolerance,
           "complex result mismatch");
}

void expect_error(const std::function<void()> &call,
                  const std::string &message) {
  try {
    call();
  } catch (const std::invalid_argument &) {
    return;
  }
  throw std::runtime_error(message);
}

power::Net two_bus(power::BusKind second = power::BusKind::PQ) {
  power::Net net;
  net.base = {100.0e6, 230.0e3};
  net.buses.kind = {power::BusKind::Slack, second};
  net.buses.p = pluss::series::make({0.0, -0.1});
  net.buses.q = pluss::series::make({0.0, -0.05});
  net.buses.vm = pluss::series::make({1.0, 1.0});
  net.buses.va = pluss::series::make({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = pluss::series::make({0.0});
  net.lines.x = pluss::series::make({0.1});
  return net;
}

power::Net triangle(bool batched = false) {
  power::Net net;
  net.base = {100.0e6, 230.0e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ,
                    power::BusKind::PQ};
  net.buses.p =
      batched
          ? pluss::series::make(
                std::vector<double>{0.3, -0.1, -0.2, 0.45, -0.15, -0.3}, {2, 3})
          : pluss::series::make({0.3, -0.1, -0.2});
  net.buses.q = pluss::series::zeros_like(net.buses.p);
  net.buses.vm = pluss::series::ones_like(net.buses.p);
  net.buses.va = pluss::series::zeros_like(net.buses.p);
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = pluss::series::make({0.0, 0.0, 0.0});
  net.lines.x = pluss::series::make({1.0, 1.0, 1.0});
  return net;
}

power::Net dispatch_net(bool batched = false) {
  auto net = two_bus();
  net.buses.p = batched ? pluss::series::make(
                              std::vector<double>{0.0, -1.0, 0.0, -1.5}, {2, 2})
                        : pluss::series::make({0.0, -1.0});
  net.buses.q = pluss::series::zeros_like(net.buses.p);
  net.gens.bus = {0, 1};
  net.gens.p = pluss::series::make({0.0, 0.0});
  net.gens.q = pluss::series::make({0.0, 0.0});
  return net;
}

power::Net secure_dispatch_net() {
  auto net = triangle();
  net.buses.p = pluss::series::make({0.0, -1.0, 0.0});
  net.buses.q = pluss::series::zeros_like(net.buses.p);
  net.gens.bus = {0, 2};
  net.gens.p = pluss::series::make({0.0, 0.0});
  net.gens.q = pluss::series::make({0.0, 0.0});
  return net;
}

power::GenCost dispatch_cost() {
  return {pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0}),
          pluss::series::make({0.0, 0.0}), pluss::series::make({1.0, 2.0})};
}

power::DispatchOptions precise_dispatch(bool secure = false) {
  power::DispatchOptions options;
  options.secure = secure;
  options.qp.abs_tol = 1e-9;
  options.qp.rel_tol = 1e-9;
  options.qp.check_every = 5;
  return options;
}

power::Net opf_net(bool batched = false) {
  auto net = two_bus();
  net.buses.p = batched ? pluss::series::make(
                              std::vector<double>{0.0, -1.0, 0.0, -1.4}, {2, 2})
                        : pluss::series::make({0.0, -1.0});
  net.buses.q = batched ? pluss::series::make(
                              std::vector<double>{0.0, -0.2, 0.0, -0.3}, {2, 2})
                        : pluss::series::make({0.0, -0.2});
  net.buses.vm = pluss::series::ones_like(net.buses.p);
  net.buses.va = pluss::series::zeros_like(net.buses.vm);
  net.gens.bus = {0, 1};
  net.gens.p = pluss::series::make({0.7, 0.3});
  net.gens.q = pluss::series::make({0.1, 0.1});
  return net;
}

power::GenCost opf_cost() {
  power::GenCost cost{
      pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0}),
      pluss::series::make({0.0, 0.0}), pluss::series::make({1.0, 2.0})};
  cost.q_linear = pluss::series::make({0.0, 0.0});
  cost.q_quadratic = pluss::series::make({0.1, 0.2});
  return cost;
}

power::Limits
opf_limits(double flow = std::numeric_limits<double>::infinity()) {
  power::Limits limits{
      pluss::series::make({-1.0, -1.0}), pluss::series::make({1.0, 1.0}),
      pluss::series::make({0.9, 0.9}), pluss::series::make({1.1, 1.1})};
  if (std::isfinite(flow))
    limits.flow = pluss::series::make({flow});
  return limits;
}

power::Measurements all_measurements(const power::AcResult &state,
                                     std::int64_t buses, std::int64_t lines) {
  power::Measurements measurements;
  const auto append = [&](power::MeasureKind kind, std::int64_t count) {
    for (std::int64_t index = 0; index < count; ++index) {
      measurements.kind.push_back(kind);
      measurements.index.push_back(index);
    }
  };
  append(power::MeasureKind::FromP, lines);
  append(power::MeasureKind::ToP, lines);
  append(power::MeasureKind::BusP, buses);
  append(power::MeasureKind::Angle, buses);
  append(power::MeasureKind::FromQ, lines);
  append(power::MeasureKind::ToQ, lines);
  append(power::MeasureKind::BusQ, buses);
  append(power::MeasureKind::Magnitude, buses);
  measurements.value = state.from_power.real().unite(
      {state.to_power.real(), state.injection.real(), state.angle,
       state.from_power.imag(), state.to_power.imag(), state.injection.imag(),
       state.magnitude},
      -1);
  measurements.stddev = pluss::series::full_like(measurements.value, 0.01);
  return measurements;
}

} // namespace

int main() {
  const power::Base base{100.0e6, 230.0e3};
  expect(std::abs(power::zbase(base) - 529.0) < 1e-12,
         "impedance base is wrong");
  expect(std::abs(power::ybase(base) - 1.0 / 529.0) < 1e-15,
         "admittance base is wrong");
  expect(std::abs(power::ibase(base) - 100.0e6 / (std::sqrt(3.0) * 230.0e3)) <
             1e-12,
         "current base is wrong");
  close(power::pu(pluss::series::make({52.9}), power::zbase(base)), {0.1});
  close(power::si(pluss::series::make({0.1}), power::zbase(base)), {52.9});

  const auto grid = power::make(two_bus());
  close_complex(grid.ybus(),
                {{0.0, -10.0}, {0.0, 10.0}, {0.0, 10.0}, {0.0, -10.0}});

  auto stamped_net = two_bus();
  stamped_net.buses.g = pluss::series::make({0.01, 0.02});
  stamped_net.buses.b = pluss::series::make({-0.03, 0.04});
  stamped_net.lines.r = pluss::series::make({0.02});
  stamped_net.lines.x = pluss::series::make({0.04});
  stamped_net.lines.charge = pluss::series::make({0.06});
  stamped_net.lines.tap = pluss::series::make({1.1});
  stamped_net.lines.shift = pluss::series::make({0.2});
  const auto stamped = power::make(stamped_net);
  const std::complex<double> ys = 1.0 / std::complex<double>{0.02, 0.04};
  const std::complex<double> tap = std::polar(1.1, 0.2);
  const std::complex<double> ytt = ys + std::complex<double>{0.0, 0.03};
  const std::complex<double> yff = ytt / (1.1 * 1.1);
  const std::complex<double> yft = -ys / std::conj(tap);
  const std::complex<double> ytf = -ys / tap;
  close_complex(stamped.yfrom(), {yff, yft});
  close_complex(stamped.yto(), {ytf, ytt});
  close_complex(stamped.ybus(), {yff + std::complex<double>{0.01, -0.03}, yft,
                                 ytf, ytt + std::complex<double>{0.02, 0.04}});

  const auto linear = power::dc(grid);
  close(linear.angle, {0.0, -0.01});
  close(linear.injection, {0.1, -0.1});
  close(linear.from_power, {0.1});
  close(linear.to_power, {-0.1});
  expect(linear.residual.item<double>() < 1e-12,
         "DC balance residual is too large");

  const auto nonlinear = power::ac(grid);
  const double real_voltage = (1.0 + std::sqrt(0.9796)) / 2.0;
  const double expected_magnitude =
      std::sqrt(real_voltage * real_voltage + 0.0001);
  close_complex(nonlinear.voltage, {{1.0, 0.0}, {real_voltage, -0.01}}, 1e-9);
  close(nonlinear.magnitude, {1.0, expected_magnitude}, 1e-9);
  expect(nonlinear.converged.item<bool>(), "PQ flow did not converge");
  expect(nonlinear.residual.item<double>() < 1e-10,
         "AC balance residual is too large");
  close(nonlinear.from_power.real(), {0.1}, 1e-9);
  close(nonlinear.to_power.real(), {-0.1}, 1e-9);

  auto pv_net = two_bus(power::BusKind::PV);
  pv_net.buses.p = pluss::series::make({0.0, 0.0});
  pv_net.buses.q = pluss::series::make({0.0, 0.0});
  pv_net.gens.bus = {1};
  pv_net.gens.p = pluss::series::make({0.1});
  pv_net.gens.q = pluss::series::make({0.0});
  const auto pv = power::ac(power::make(pv_net));
  close(pv.magnitude, {1.0, 1.0});
  close(pv.angle, {0.0, std::asin(0.01)}, 1e-10);
  expect(pv.converged.item<bool>(), "PV flow did not converge");
  expect(power::make(pv_net).gens() == 1,
         "generator topology was not retained");

  auto batch_net = two_bus();
  batch_net.buses.p =
      pluss::series::make(std::vector<double>{0.0, -0.1, 0.0, -0.2}, {2, 2});
  batch_net.buses.q =
      pluss::series::make(std::vector<double>{0.0, -0.05, 0.0, -0.08}, {2, 2});
  const auto batch = power::ac(power::make(batch_net));
  expect(batch.voltage.sizes() == std::vector<std::int64_t>{2, 2},
         "scenario batch shape was not retained");
  expect(batch.converged.all().item<bool>(), "batched flow did not converge");
  expect(batch.residual.lt(1e-10).all().item<bool>(),
         "batched balance residual is too large");

  auto tracked_p = pluss::series::make({0.0, -0.1});
  tracked_p.tracks_slope_(true);
  auto sensitivity_net = two_bus();
  sensitivity_net.buses.p = tracked_p;
  power::dc(power::make(sensitivity_net)).angle.select(-1, 1).reverse();
  close(tracked_p.sensitivity(), {0.0, 0.1}, 1e-9);

  auto tracked_ac_p = pluss::series::make({0.0, -0.1});
  tracked_ac_p.tracks_slope_(true);
  auto ac_sensitivity_net = two_bus();
  ac_sensitivity_net.buses.p = tracked_ac_p;
  power::ac(power::make(ac_sensitivity_net)).magnitude.select(-1, 1).reverse();
  const double b_imag = -0.01;
  const double da_dp = -0.2 * b_imag / std::sqrt(0.9796);
  const double dvm_dp =
      (real_voltage * da_dp + b_imag * 0.1) / expected_magnitude;
  close(tracked_ac_p.sensitivity(), {0.0, dvm_dp}, 1e-7);

  const auto mesh = power::make(triangle());
  close(power::ptdf(mesh), {0.0, -2.0 / 3.0, -1.0 / 3.0, 0.0, 1.0 / 3.0,
                            -1.0 / 3.0, 0.0, -1.0 / 3.0, -2.0 / 3.0});
  const auto mesh_lodf = power::lodf(mesh);
  close(mesh_lodf.factor, {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0, -1.0});
  expect(mesh_lodf.valid.all().item<bool>(),
         "meshed line outages were marked invalid");

  const auto screen = power::outages(mesh);
  close(screen.base_flow, {2.0 / 15.0, 1.0 / 30.0, 1.0 / 6.0});
  close(screen.flow, {0.0, -0.1, 0.3, 0.1, 0.0, 0.2, 0.3, 0.2, 0.0});
  expect(screen.flow.sizes() == std::vector<std::int64_t>{3, 3},
         "outage and monitored-line axes are reversed");

  for (std::int64_t line = 0; line < 3; ++line) {
    auto removed = triangle();
    removed.lines.active = {true, true, true};
    removed.lines.active[static_cast<std::size_t>(line)] = false;
    const auto direct = power::dc(power::make(removed)).from_power;
    close(screen.flow.select(-2, line), direct.to_vec<double>());
  }

  const auto batch_screen = power::outages(power::make(triangle(true)));
  expect(batch_screen.flow.sizes() == std::vector<std::int64_t>{2, 3, 3},
         "batched contingency shape is wrong");
  close(batch_screen.flow, {0.0, -0.1, 0.3, 0.1, 0.0, 0.2, 0.3, 0.2, 0.0, 0.0,
                            -0.15, 0.45, 0.15, 0.0, 0.3, 0.45, 0.3, 0.0});

  const auto radial_lodf = power::lodf(grid);
  expect(!radial_lodf.valid.item<bool>(),
         "bridge outage was reported as connected");
  expect(radial_lodf.factor.isnan().all().item<bool>(),
         "bridge LODF was not represented as unavailable");
  expect(power::outages(grid).flow.isnan().all().item<bool>(),
         "islanding outage produced finite branch flows");

  auto inactive_net = triangle();
  inactive_net.lines.active = {false, true, true};
  const auto inactive = power::outages(power::make(inactive_net));
  expect(!inactive.valid.any().item<bool>(),
         "an inactive or now-bridging line was treated as a viable outage");
  expect(inactive.flow.isnan().all().item<bool>(),
         "unavailable outages did not produce explicit NaN rows");

  auto tracked_mesh_p = pluss::series::make({0.3, -0.1, -0.2});
  tracked_mesh_p.tracks_slope_(true);
  auto tracked_mesh = triangle();
  tracked_mesh.buses.p = tracked_mesh_p;
  power::outages(power::make(tracked_mesh))
      .flow.select(-2, 0)
      .select(-1, 2)
      .reverse();
  close(tracked_mesh_p.sensitivity(), {0.0, -1.0, -1.0});

  const auto economic =
      power::dispatch(power::make(dispatch_net()), dispatch_cost(),
                      pluss::series::make({2.0}), precise_dispatch());
  expect(economic.converged, "economic dispatch did not converge");
  close(economic.generation, {2.0 / 3.0, 1.0 / 3.0}, 2e-7);
  close(economic.injection, {2.0 / 3.0, -2.0 / 3.0}, 2e-7);
  close(economic.flow, {2.0 / 3.0}, 2e-7);
  close(economic.objective, {2.0 / 3.0}, 2e-7);
  expect(!economic.post_flow.defined() && !economic.outage_valid.defined(),
         "ordinary dispatch materialized contingency results");

  auto open_cost = dispatch_cost();
  open_cost.upper =
      pluss::series::make({std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::infinity()});
  const auto unbounded =
      power::dispatch(power::make(dispatch_net()), open_cost,
                      pluss::series::make({2.0}), precise_dispatch());
  close(unbounded.generation, economic.generation.to_vec<double>(), 2e-7);

  const auto congested =
      power::dispatch(power::make(dispatch_net()), dispatch_cost(),
                      pluss::series::make({0.4}), precise_dispatch());
  expect(congested.converged, "congested dispatch did not converge");
  close(congested.generation, {0.4, 0.6}, 2e-7);
  close(congested.flow, {0.4}, 2e-7);
  close(congested.objective, {0.88}, 3e-7);

  auto inactive_dispatch = dispatch_net();
  inactive_dispatch.gens.active = {true, false};
  const auto inactive_gen =
      power::dispatch(power::make(inactive_dispatch), dispatch_cost(),
                      pluss::series::make({2.0}), precise_dispatch());
  expect(inactive_gen.converged,
         "inactive-generator dispatch did not converge");
  close(inactive_gen.generation, {1.0, 0.0}, 2e-7);

  const auto batch_dispatch =
      power::dispatch(power::make(dispatch_net(true)), dispatch_cost(),
                      pluss::series::make({2.0}), precise_dispatch());
  expect(batch_dispatch.converged, "batched dispatch did not converge");
  expect(batch_dispatch.generation.sizes() == std::vector<std::int64_t>{2, 2},
         "dispatch scenario axes were not retained");
  close(batch_dispatch.generation, {2.0 / 3.0, 1.0 / 3.0, 1.0, 0.5}, 3e-7);
  close(batch_dispatch.flow, {2.0 / 3.0, 1.0}, 3e-7);
  close(batch_dispatch.objective, {2.0 / 3.0, 1.5}, 4e-7);

  const power::GenCost secure_cost{
      pluss::series::make({0.0, 0.0}), pluss::series::make({1.0, 1.0}),
      pluss::series::make({0.0, 0.0}), pluss::series::make({0.1, 1.0})};
  const auto secure_grid = power::make(secure_dispatch_net());
  const auto base_dispatch =
      power::dispatch(secure_grid, secure_cost,
                      pluss::series::make({1.1, 1.1, 0.8}), precise_dispatch());
  close(base_dispatch.generation, {10.0 / 11.0, 1.0 / 11.0}, 3e-7);
  const auto secure_dispatch = power::dispatch(
      secure_grid, secure_cost, pluss::series::make({1.1, 1.1, 0.8}),
      precise_dispatch(true));
  expect(secure_dispatch.converged,
         "preventive security-constrained dispatch did not converge");
  close(secure_dispatch.generation, {0.8, 0.2}, 4e-7);
  expect(secure_dispatch.post_flow.sizes() == std::vector<std::int64_t>{3, 3},
         "security dispatch contingency axes are wrong");
  expect(secure_dispatch.outage_valid.all().item<bool>(),
         "meshed security dispatch discarded a viable outage");
  expect(secure_dispatch.post_flow.abs()
             .le(pluss::series::make({1.1, 1.1, 0.8}).unsqueeze(-2).add(1e-7))
             .all()
             .item<bool>(),
         "security dispatch violates a post-outage line limit");

  const auto radial_secure =
      power::dispatch(power::make(dispatch_net()), dispatch_cost(),
                      pluss::series::make({2.0}), precise_dispatch(true));
  expect(!radial_secure.outage_valid.item<bool>() &&
             radial_secure.post_flow.isnan().all().item<bool>(),
         "bridge contingency was enforced as a connected network");
  close(radial_secure.generation, economic.generation.to_vec<double>(), 2e-7);

  auto tracked_load = pluss::series::make({0.0, -1.0});
  tracked_load.tracks_slope_(true);
  auto differentiated_net = dispatch_net();
  differentiated_net.buses.p = tracked_load;
  power::dispatch(power::make(differentiated_net), dispatch_cost(),
                  pluss::series::make({2.0}), precise_dispatch())
      .objective.reverse();
  expect(tracked_load.has_sensitivity() &&
             tracked_load.sensitivity().isfinite().all().item<bool>(),
         "dispatch objective detached from fixed injections");
  close(tracked_load.sensitivity(), {-4.0 / 3.0, -4.0 / 3.0}, 3e-4);

  const auto ac_opt =
      power::opf(power::make(opf_net()), opf_cost(), opf_limits());
  expect(ac_opt.converged,
         "AC optimal power flow did not converge: " + ac_opt.message);
  close(ac_opt.generation_p, {2.0 / 3.0, 1.0 / 3.0}, 1e-3);
  expect(ac_opt.generation_q.ge(-1.0).all().item<bool>() &&
             ac_opt.generation_q.le(1.0).all().item<bool>(),
         "AC OPF violates reactive generation bounds");
  expect(ac_opt.magnitude.ge(0.9 - 1e-7).all().item<bool>() &&
             ac_opt.magnitude.le(1.1 + 1e-7).all().item<bool>(),
         "AC OPF violates voltage bounds");
  const auto opf_p_balance = ac_opt.injection.real().sub(
      pluss::series::make({0.0, -1.0}).add(ac_opt.generation_p));
  const auto opf_q_balance = ac_opt.injection.imag().sub(
      pluss::series::make({0.0, -0.2}).add(ac_opt.generation_q));
  expect(std::max(opf_p_balance.abs().amax().item<double>(),
                  opf_q_balance.abs().amax().item<double>()) < 2e-7,
         "AC OPF violates complex nodal balance");
  expect(ac_opt.p_price.sizes() == std::vector<std::int64_t>{2} &&
             ac_opt.q_price.sizes() == std::vector<std::int64_t>{2} &&
             ac_opt.from_shadow.sizes() == std::vector<std::int64_t>{1} &&
             ac_opt.to_shadow.sizes() == std::vector<std::int64_t>{1},
         "AC OPF multiplier axes are wrong");

  const auto batched_opf =
      power::opf(power::make(opf_net(true)), opf_cost(), opf_limits());
  expect(batched_opf.converged &&
             batched_opf.generation_p.sizes() ==
                 std::vector<std::int64_t>{2, 2} &&
             batched_opf.objective.sizes() == std::vector<std::int64_t>{2},
         "batched AC OPF lost scenario axes");
  close(batched_opf.generation_p,
        {2.0 / 3.0, 1.0 / 3.0, 14.0 / 15.0, 7.0 / 15.0}, 2e-3);

  const auto rated_opf =
      power::opf(power::make(opf_net()), opf_cost(), opf_limits(0.4));
  expect(rated_opf.converged, "rated AC OPF did not converge");
  expect(rated_opf.from_power.abs().le(0.4 + 2e-7).all().item<bool>() &&
             rated_opf.to_power.abs().le(0.4 + 2e-7).all().item<bool>(),
         "AC OPF violates a branch-end apparent-power rating");
  expect(rated_opf.from_shadow.add(rated_opf.to_shadow)
             .gt(1e-7)
             .any()
             .item<bool>(),
         "binding AC branch rating has no multiplier");

  auto inactive_opf_net = opf_net();
  inactive_opf_net.gens.active = {true, false};
  inactive_opf_net.gens.p = pluss::series::make({1.0, 0.0});
  inactive_opf_net.gens.q = pluss::series::make({0.25, 0.0});
  const auto inactive_opf =
      power::opf(power::make(inactive_opf_net), opf_cost(), opf_limits());
  expect(inactive_opf.converged,
         "AC OPF with an inactive generator did not converge");
  close(inactive_opf.generation_p, {1.0, 0.0}, 2e-6);
  close(inactive_opf.generation_q.select(-1, 1), {0.0}, 2e-7);

  auto float_opf_net = opf_net();
  float_opf_net.buses.p = float_opf_net.buses.p.to(pluss::ScalarType::Float32);
  float_opf_net.buses.q = float_opf_net.buses.q.to(pluss::ScalarType::Float32);
  float_opf_net.buses.vm =
      float_opf_net.buses.vm.to(pluss::ScalarType::Float32);
  float_opf_net.buses.va =
      float_opf_net.buses.va.to(pluss::ScalarType::Float32);
  float_opf_net.gens.p = float_opf_net.gens.p.to(pluss::ScalarType::Float32);
  float_opf_net.gens.q = float_opf_net.gens.q.to(pluss::ScalarType::Float32);
  float_opf_net.lines.r = float_opf_net.lines.r.to(pluss::ScalarType::Float32);
  float_opf_net.lines.x = float_opf_net.lines.x.to(pluss::ScalarType::Float32);
  auto float_cost = opf_cost();
  float_cost.lower = float_cost.lower.to(pluss::ScalarType::Float32);
  float_cost.upper = float_cost.upper.to(pluss::ScalarType::Float32);
  float_cost.linear = float_cost.linear.to(pluss::ScalarType::Float32);
  float_cost.quadratic = float_cost.quadratic.to(pluss::ScalarType::Float32);
  float_cost.q_linear = float_cost.q_linear.to(pluss::ScalarType::Float32);
  float_cost.q_quadratic =
      float_cost.q_quadratic.to(pluss::ScalarType::Float32);
  auto float_limits = opf_limits();
  float_limits.q_lower = float_limits.q_lower.to(pluss::ScalarType::Float32);
  float_limits.q_upper = float_limits.q_upper.to(pluss::ScalarType::Float32);
  float_limits.vm_lower = float_limits.vm_lower.to(pluss::ScalarType::Float32);
  float_limits.vm_upper = float_limits.vm_upper.to(pluss::ScalarType::Float32);
  const auto float_opf =
      power::opf(power::make(float_opf_net), float_cost, float_limits);
  expect(float_opf.converged &&
             float_opf.generation_p.scalar_type() ==
                 pluss::ScalarType::Float32 &&
             float_opf.magnitude.scalar_type() == pluss::ScalarType::Float32,
         "float32 AC OPF changed dtype or failed to converge");

  auto tracked_cost = opf_cost();
  tracked_cost.quadratic = pluss::series::make({1.0, 2.0});
  tracked_cost.quadratic.tracks_slope_(true);
  const auto tracked_opf =
      power::opf(power::make(opf_net()), tracked_cost, opf_limits());
  tracked_opf.objective.sum().reverse();
  expect(tracked_cost.quadratic.has_sensitivity() &&
             tracked_cost.quadratic.sensitivity().isfinite().all().item<bool>(),
         "AC OPF objective detached from cost coefficients");
  close(tracked_cost.quadratic.sensitivity(),
        tracked_opf.generation_p.square().to_vec<double>(), 2e-4);

  const auto estimated_truth = power::ac(grid);
  const auto complete_measurements = all_measurements(estimated_truth, 2, 1);
  const auto estimated = power::estimate(grid, complete_measurements);
  expect(estimated.converged.item<bool>(),
         "exact AC state estimate did not converge");
  close(estimated.magnitude, estimated_truth.magnitude.to_vec<double>(), 2e-9);
  close(estimated.angle, estimated_truth.angle.to_vec<double>(), 2e-9);
  close_complex(estimated.voltage,
                estimated_truth.voltage.to(pluss::ScalarType::Complex128)
                    .to_vec<std::complex<double>>(),
                2e-9);
  expect(estimated.objective.item<double>() < 1e-16,
         "exact measurements retained weighted residual");
  expect(estimated.covariance.sizes() == std::vector<std::int64_t>{3, 3},
         "state covariance has the wrong axes");
  expect(estimated.covariance.linearalg.eigvalsh().gt(0.0).all().item<bool>(),
         "state covariance is not positive definite");

  auto reordered_measurements = complete_measurements;
  std::reverse(reordered_measurements.kind.begin(),
               reordered_measurements.kind.end());
  std::reverse(reordered_measurements.index.begin(),
               reordered_measurements.index.end());
  reordered_measurements.value = reordered_measurements.value.flip(-1);
  reordered_measurements.stddev = reordered_measurements.stddev.flip(-1);
  const auto reordered = power::estimate(grid, reordered_measurements);
  close(reordered.magnitude, estimated.magnitude.to_vec<double>(), 2e-9);
  close(reordered.angle, estimated.angle.to_vec<double>(), 2e-9);

  auto rough_start = two_bus();
  rough_start.buses.vm = pluss::series::make({1.08, 0.85});
  rough_start.buses.va = pluss::series::make({0.0, 0.2});
  const auto recovered =
      power::estimate(power::make(rough_start), complete_measurements);
  expect(recovered.converged.item<bool>(),
         "state estimate did not recover from an imperfect initial state");
  close(recovered.magnitude, estimated_truth.magnitude.to_vec<double>(), 2e-9);
  close(recovered.angle, estimated_truth.angle.to_vec<double>(), 2e-9);

  auto wrapped_measurements = complete_measurements;
  std::vector<double> angle_shift(
      static_cast<std::size_t>(wrapped_measurements.value.numel()), 0.0);
  angle_shift[5] = 2.0 * std::acos(-1.0);
  wrapped_measurements.value =
      wrapped_measurements.value.add(pluss::series::make(angle_shift));
  const auto wrapped = power::estimate(grid, wrapped_measurements);
  close(wrapped.angle, estimated.angle.to_vec<double>(), 2e-9);
  expect(wrapped.objective.item<double>() < 1e-16,
         "equivalent wrapped angle produced a residual");

  auto noisy_measurements = complete_measurements;
  std::vector<double> outlier(
      static_cast<std::size_t>(noisy_measurements.value.numel()), 0.0);
  outlier[0] = 0.2;
  noisy_measurements.value =
      noisy_measurements.value.add(pluss::series::make(outlier));
  const auto noisy = power::estimate(grid, noisy_measurements);
  expect(noisy.converged.item<bool>(),
         "redundant noisy estimate did not converge");
  expect(noisy.objective.item<double>() > 10.0 &&
             noisy.normalized_residual.abs().amax().item<double>() > 3.0,
         "gross measurement error was hidden by the estimator");

  const auto batch_truth = power::ac(power::make(batch_net));
  const auto batch_measurements = all_measurements(batch_truth, 2, 1);
  const auto batch_estimate =
      power::estimate(power::make(batch_net), batch_measurements);
  expect(batch_estimate.converged.all().item<bool>(),
         "batched state estimation did not converge");
  expect(batch_estimate.angle.sizes() == std::vector<std::int64_t>{2, 2} &&
             batch_estimate.covariance.sizes() ==
                 std::vector<std::int64_t>{2, 3, 3},
         "state-estimation scenario axes were not retained");
  close(batch_estimate.magnitude, batch_truth.magnitude.to_vec<double>(), 3e-9);
  close(batch_estimate.angle, batch_truth.angle.to_vec<double>(), 3e-9);

  auto tracked_measurements = complete_measurements;
  const std::vector<std::int64_t> measurement_shape(
      complete_measurements.value.sizes().begin(),
      complete_measurements.value.sizes().end());
  tracked_measurements.value = pluss::series::make(
      complete_measurements.value.to_vec<double>(), measurement_shape);
  tracked_measurements.value.tracks_slope_(true);
  power::estimate(grid, tracked_measurements).angle.select(-1, 1).reverse();
  expect(
      tracked_measurements.value.has_sensitivity() &&
          tracked_measurements.value.sensitivity()
              .isfinite()
              .all()
              .item<bool>() &&
          tracked_measurements.value.sensitivity().abs().sum().item<double>() >
              0.0,
      "state estimate detached from measurements");

  auto empty_net = two_bus();
  empty_net.buses.p = pluss::series::zeros_like(empty_net.buses.p);
  empty_net.buses.q = pluss::series::zeros_like(empty_net.buses.q);
  const auto empty_grid = power::make(empty_net);
  const power::LoadPath load_path{pluss::series::make({0.0, -0.1}),
                                  pluss::series::make({0.0, -0.05})};
  power::TraceOptions target_options;
  target_options.target = 1.0;
  target_options.step = 0.1;
  target_options.step_max = 0.25;
  const auto target_trace = power::trace(empty_grid, load_path, target_options);
  expect(target_trace.reached.item<bool>(),
         "continuation did not reach its target loading factor");
  expect(target_trace.points >= 2 &&
             target_trace.factor.size(-1) == target_trace.points &&
             target_trace.voltage.size(-2) == target_trace.points,
         "continuation point axes are inconsistent");
  close(target_trace.factor.select(-1, target_trace.points - 1), {1.0}, 1e-12);
  close(target_trace.magnitude.select(-2, target_trace.points - 1),
        nonlinear.magnitude.to_vec<double>(), 2e-9);
  close(target_trace.angle.select(-2, target_trace.points - 1),
        nonlinear.angle.to_vec<double>(), 2e-9);
  expect(
      target_trace.residual.select(-1, target_trace.points - 1).item<double>() <
          1e-9,
      "target continuation point violates AC balance");

  power::TraceOptions nose_options = target_options;
  nose_options.stop = power::TraceStop::Nose;
  nose_options.step_max = 0.5;
  nose_options.max_steps = 200;
  const auto nose = power::trace(empty_grid, load_path, nose_options);
  const double nose_factor = (-0.02 + std::sqrt(0.002)) / 0.0008;
  const double nose_real = 0.5;
  const double nose_imag = -0.01 * nose_factor;
  expect(nose.reached.item<bool>(),
         "continuation did not locate the voltage-stability nose");
  close(nose.factor.select(-1, nose.points - 1), {nose_factor}, 2e-6);
  close(nose.magnitude.select(-2, nose.points - 1),
        {1.0, std::hypot(nose_real, nose_imag)}, 2e-6);
  expect(std::abs(nose.tangent.select(-2, nose.points - 1)
                      .select(-1, 2)
                      .item<double>()) < 2e-6,
         "localized nose retains a non-zero loading tangent");

  power::TraceOptions unreachable_options = target_options;
  unreachable_options.target = nose_factor + 1.0;
  unreachable_options.max_steps = 200;
  const auto unreachable =
      power::trace(empty_grid, load_path, unreachable_options);
  expect(!unreachable.reached.item<bool>(),
         "continuation crossed a target beyond the steady-state limit");
  close(unreachable.factor.select(-1, unreachable.points - 1), {nose_factor},
        2e-6);

  auto empty_batch = two_bus();
  empty_batch.buses.p =
      pluss::series::make(std::vector<double>(4, 0.0), {2, 2});
  empty_batch.buses.q =
      pluss::series::make(std::vector<double>(4, 0.0), {2, 2});
  empty_batch.buses.vm =
      pluss::series::make(std::vector<double>(4, 1.0), {2, 2});
  empty_batch.buses.va =
      pluss::series::make(std::vector<double>(4, 0.0), {2, 2});
  const power::LoadPath batch_path{
      pluss::series::make(std::vector<double>{0.0, -0.1, 0.0, -0.2}, {2, 2}),
      pluss::series::make(std::vector<double>{0.0, -0.05, 0.0, -0.08}, {2, 2})};
  const auto batch_trace =
      power::trace(power::make(empty_batch), batch_path, target_options);
  expect(batch_trace.reached.all().item<bool>() &&
             batch_trace.magnitude.sizes() ==
                 std::vector<std::int64_t>{2, batch_trace.points, 2} &&
             batch_trace.valid.sizes() ==
                 std::vector<std::int64_t>{2, batch_trace.points},
         "batched continuation lost scenario or point axes");
  close(batch_trace.magnitude.select(-2, batch_trace.points - 1),
        batch.magnitude.to_vec<double>(), 3e-9);

  const auto batch_nose =
      power::trace(power::make(empty_batch), batch_path, nose_options);
  const double second_nose =
      (-0.032 + std::sqrt(0.032 * 0.032 + 4.0 * 0.0016)) / (2.0 * 0.0016);
  expect(batch_nose.reached.all().item<bool>(),
         "batched continuation did not localize every scenario nose");
  close(batch_nose.factor.select(-1, batch_nose.points - 1),
        {nose_factor, second_nose}, 3e-6);
  expect(batch_nose.tangent.select(-2, batch_nose.points - 1)
             .select(-1, 2)
             .abs()
             .lt(3e-6)
             .all()
             .item<bool>(),
         "a batched nose retains a non-zero loading tangent");
  expect(!batch_nose.valid.select(-1, batch_nose.points - 1).all().item<bool>(),
         "a completed batch lane was not masked while its peer continued");

  auto pv_base = two_bus(power::BusKind::PV);
  pv_base.buses.p = pluss::series::zeros_like(pv_base.buses.p);
  pv_base.buses.q = pluss::series::zeros_like(pv_base.buses.q);
  const auto pv_trace =
      power::trace(power::make(pv_base),
                   power::LoadPath{pluss::series::make({0.0, 0.1}),
                                   pluss::series::make({0.0, 0.0})},
                   target_options);
  expect(pv_trace.reached.item<bool>(),
         "PV-only continuation did not reach its target");
  close(pv_trace.magnitude.select(-2, pv_trace.points - 1), {1.0, 1.0});
  close(pv_trace.angle.select(-2, pv_trace.points - 1), {0.0, std::asin(0.01)},
        2e-9);

  auto float_net = empty_net;
  float_net.buses.p = float_net.buses.p.to(pluss::ScalarType::Float32);
  float_net.buses.q = float_net.buses.q.to(pluss::ScalarType::Float32);
  float_net.buses.vm = float_net.buses.vm.to(pluss::ScalarType::Float32);
  float_net.buses.va = float_net.buses.va.to(pluss::ScalarType::Float32);
  float_net.lines.r = float_net.lines.r.to(pluss::ScalarType::Float32);
  float_net.lines.x = float_net.lines.x.to(pluss::ScalarType::Float32);
  auto float_options = target_options;
  float_options.tolerance = 2e-6;
  const auto float_grid = power::make(float_net);
  (void)power::ac(float_grid, power::Options{.tolerance = 2e-6});
  auto float_loaded = float_net;
  float_loaded.buses.p = load_path.p.to(pluss::ScalarType::Float32);
  float_loaded.buses.q = load_path.q.to(pluss::ScalarType::Float32);
  const auto float_ac =
      power::ac(power::make(float_loaded), power::Options{.tolerance = 2e-6});
  expect(float_ac.converged.item<bool>() &&
             float_ac.magnitude.scalar_type() == pluss::ScalarType::Float32,
         "float32 AC Newton solve promoted its state");
  const auto float_trace =
      power::trace(float_grid,
                   power::LoadPath{load_path.p.to(pluss::ScalarType::Float32),
                                   load_path.q.to(pluss::ScalarType::Float32)},
                   float_options);
  expect(float_trace.reached.item<bool>() &&
             float_trace.magnitude.scalar_type() == pluss::ScalarType::Float32,
         "float32 continuation changed dtype or missed its target");
  close(float_trace.magnitude.select(-2, float_trace.points - 1),
        nonlinear.magnitude.to_vec<double>(), 2e-5);

  auto tracked_target = pluss::series::make({0.0, -0.1});
  tracked_target.tracks_slope_(true);
  const auto tracked_trace = power::trace(
      empty_grid,
      power::LoadPath{tracked_target, pluss::series::make({0.0, -0.05})},
      target_options);
  tracked_trace.magnitude.select(-2, tracked_trace.points - 1)
      .select(-1, 1)
      .reverse();
  expect(tracked_target.has_sensitivity() &&
             tracked_target.sensitivity().isfinite().all().item<bool>() &&
             tracked_target.sensitivity().abs().sum().item<double>() > 0.0,
         "continuation target detached from reverse sensitivity");

  expect_error(
      [] {
        auto bad = two_bus();
        bad.buses.kind = {power::BusKind::PQ, power::BusKind::PQ};
        (void)power::make(bad);
      },
      "network without a slack bus was accepted");
  expect_error(
      [] {
        auto bad = two_bus();
        bad.lines.x = pluss::series::make({0.0});
        (void)power::make(bad);
      },
      "zero line impedance was accepted");
  expect_error(
      [] {
        auto bad = two_bus();
        bad.lines.active = {false};
        (void)power::make(bad);
      },
      "an island disconnected from the slack was accepted");
  expect_error([&] { (void)power::ac(grid, power::Options{.tolerance = 0.0}); },
               "invalid Newton tolerance was accepted");
  expect_error([&] { (void)power::lodf(mesh, 0.0); },
               "invalid outage tolerance was accepted");
  expect_error(
      [&] {
        auto bad = dispatch_cost();
        bad.quadratic = pluss::series::make({1.0, -1.0});
        (void)power::dispatch(power::make(dispatch_net()), bad,
                              pluss::series::make({2.0}));
      },
      "negative quadratic generation cost was accepted");
  expect_error(
      [&] {
        auto bad = dispatch_cost();
        bad.upper = pluss::series::make(
            {2.0, std::numeric_limits<double>::quiet_NaN()});
        (void)power::dispatch(power::make(dispatch_net()), bad,
                              pluss::series::make({2.0}));
      },
      "NaN generator bound was accepted");
  expect_error(
      [&] {
        (void)power::dispatch(power::make(dispatch_net()), dispatch_cost(),
                              pluss::series::make({0.0}));
      },
      "non-positive line limit was accepted");
  expect_error(
      [&] {
        auto bad = dispatch_net();
        bad.gens.active = {false, false};
        (void)power::dispatch(power::make(bad), dispatch_cost(),
                              pluss::series::make({2.0}));
      },
      "dispatch without an active generator was accepted");
  expect_error(
      [&] {
        auto bad = opf_limits();
        bad.vm_lower = pluss::series::make({1.2, 0.9});
        (void)power::opf(power::make(opf_net()), opf_cost(), bad);
      },
      "AC OPF accepted a lower voltage bound above its upper bound");
  expect_error(
      [&] {
        auto bad = opf_limits();
        bad.flow = pluss::series::make({0.0});
        (void)power::opf(power::make(opf_net()), opf_cost(), bad);
      },
      "AC OPF accepted a non-positive branch rating");
  expect_error(
      [&] {
        auto bad = opf_cost();
        bad.q_quadratic = pluss::series::make({0.1, -0.2});
        (void)power::opf(power::make(opf_net()), bad, opf_limits());
      },
      "AC OPF accepted a concave reactive generation cost");
  expect_error(
      [&] {
        auto bad = opf_net();
        bad.gens.active = {false, false};
        (void)power::opf(power::make(bad), opf_cost(), opf_limits());
      },
      "AC OPF accepted an entirely inactive generator fleet");
  expect_error(
      [&] {
        power::OpfOptions bad;
        bad.constraint_tolerance = 0.0;
        (void)power::opf(power::make(opf_net()), opf_cost(), opf_limits(), bad);
      },
      "AC OPF accepted a non-positive solver tolerance");
  expect_error(
      [&] {
        auto bad = complete_measurements;
        bad.kind.resize(2);
        bad.index.resize(2);
        bad.value = bad.value.slice(-1, 0, 2);
        bad.stddev = bad.stddev.slice(-1, 0, 2);
        (void)power::estimate(grid, bad);
      },
      "under-measured state was accepted");
  expect_error(
      [&] {
        power::Measurements bad;
        bad.kind = {power::MeasureKind::Magnitude,
                    power::MeasureKind::Magnitude,
                    power::MeasureKind::Magnitude};
        bad.index = {0, 0, 0};
        bad.value = pluss::series::make({1.0, 1.0, 1.0});
        bad.stddev = pluss::series::make({0.01, 0.01, 0.01});
        (void)power::estimate(grid, bad);
      },
      "unobservable measurement set was accepted");
  expect_error(
      [&] {
        auto bad = complete_measurements;
        bad.stddev = pluss::series::zeros_like(bad.stddev);
        (void)power::estimate(grid, bad);
      },
      "zero measurement standard deviation was accepted");
  expect_error(
      [&] {
        auto bad = complete_measurements;
        bad.index[0] = 1;
        (void)power::estimate(grid, bad);
      },
      "out-of-range measurement index was accepted");
  expect_error(
      [&] {
        const power::LoadPath zero{pluss::series::zeros({2}),
                                   pluss::series::zeros({2})};
        (void)power::trace(empty_grid, zero);
      },
      "zero continuation direction was accepted");
  expect_error(
      [&] {
        auto bad = target_options;
        bad.step_min = bad.step * 2.0;
        (void)power::trace(empty_grid, load_path, bad);
      },
      "inconsistent continuation step bounds were accepted");
  expect_error(
      [&] {
        auto bad = target_options;
        bad.adapt_damping = 1.1;
        (void)power::trace(empty_grid, load_path, bad);
      },
      "invalid continuation damping was accepted");
  expect_error(
      [&] {
        const power::LoadPath bad{pluss::series::zeros({3}),
                                  pluss::series::zeros({3})};
        (void)power::trace(empty_grid, bad);
      },
      "continuation path with the wrong bus axis was accepted");

  std::cout << "Power-system foundation checks passed\n";
}
