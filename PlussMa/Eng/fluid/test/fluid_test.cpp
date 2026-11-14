#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fluid = pluss::subsp::eng::fluid;

namespace {

void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}

void close(const pluss::Series &value, const std::vector<double> &expected,
           double tolerance = 1e-8) {
  const auto actual = value.to(pluss::ScalarType::Float64).host<double>();
  expect(actual.size() == expected.size(), "result size mismatch");
  for (std::size_t lane = 0; lane < actual.size(); ++lane) {
    if (std::abs(actual[lane] - expected[lane]) > tolerance)
      throw std::runtime_error("result mismatch at lane " +
                               std::to_string(lane) + ": got " +
                               std::to_string(actual[lane]) + ", expected " +
                               std::to_string(expected[lane]));
  }
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

void expect_runtime_error(const std::function<void()> &call,
                          const std::string &message) {
  try {
    call();
  } catch (const std::runtime_error &) {
    return;
  }
  throw std::runtime_error(message);
}

fluid::Net one_pipe(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = pluss::series::make({0.0, 5.0});
  net.nodes.demand = pluss::series::make({0.0, demand});
  net.nodes.head = pluss::series::make({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = pluss::series::make({1000.0});
  net.pipes.diameter = pluss::series::make({0.30});
  net.pipes.roughness = pluss::series::make({1.0e-4});
  net.pipes.minor = pluss::series::make({0.0});
  return net;
}

fluid::Net one_pump() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = pluss::series::make({0.0, 2.0});
  net.nodes.demand = pluss::series::make({0.0, 0.01});
  net.nodes.head = pluss::series::make({10.0, 30.0});
  net.pumps.from = {0};
  net.pumps.to = {1};
  net.pumps.shutoff_head = pluss::series::make({30.0});
  net.pumps.resistance = pluss::series::make({10000.0});
  net.pumps.exponent = pluss::series::make({2.0});
  net.pumps.speed = pluss::series::make({1.0});
  return net;
}

fluid::Net tank_pipe(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Tank, fluid::NodeKind::Junction};
  net.nodes.elevation = pluss::series::make({0.0, 0.0});
  net.nodes.demand = pluss::series::make({0.0, demand});
  net.nodes.head = pluss::series::make({10.0, 9.9});
  net.tanks.area = pluss::series::make({10.0});
  net.tanks.min_level = pluss::series::make({0.0});
  net.tanks.max_level = pluss::series::make({20.0});
  net.tanks.overflow = {false};
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = pluss::series::make({10.0});
  net.pipes.diameter = pluss::series::make({0.5});
  net.pipes.roughness = pluss::series::make({1e-4});
  return net;
}

double swamee_jain(double flow, double diameter, double roughness,
                   double viscosity) {
  const double re =
      4.0 * std::abs(flow) / (3.14159265358979323846 * diameter * viscosity);
  const double y = roughness / (3.7 * diameter) + 5.74 / std::pow(re, 0.9);
  return 0.25 / std::pow(std::log10(y), 2.0);
}

double epanet_transition(double flow, double diameter, double roughness,
                         double viscosity) {
  constexpr double a2 = 500.0 * 3.14159265358979323846;
  constexpr double a9 = -0.8685889638065036553;
  constexpr double ab = 0.0032889547634539906;
  constexpr double ac = -0.00514214965799059;
  const double w = std::abs(flow) / (viscosity * diameter);
  const double y2 = roughness / (3.7 * diameter) + ab;
  const double y3 = a9 * std::log(y2);
  const double fa = 1.0 / (y3 * y3);
  const double fb = (2.0 + ac / (y2 * y3)) * fa;
  const double r = w / a2;
  const double x1 = 7.0 * fa - fb;
  const double x2 = 0.128 - 17.0 * fa + 2.5 * fb;
  const double x3 = -0.128 + 13.0 * fa - 2.0 * fb;
  const double x4 = 0.032 - 3.0 * fa + 0.5 * fb;
  return x1 + r * (x2 + r * (x3 + r * x4));
}

} // namespace

int main() {
  constexpr double pi = 3.14159265358979323846;
  constexpr double gravity = 9.80665;
  constexpr double density = 998.2;
  constexpr double viscosity = 1.0038e-6;

  const auto compiled = fluid::network(one_pipe());
  expect(compiled.nodes() == 2 && compiled.pipes() == 1 &&
             compiled.junctions() == 1 && compiled.reservoirs() == 1,
         "compiled topology counts are wrong");
  const auto result = fluid::steady(compiled);
  const double friction = swamee_jain(0.01, 0.30, 1.0e-4, viscosity);
  const double expected_loss = friction * (1000.0 / 0.30) *
                               std::pow(4.0 * 0.01 / (pi * 0.30 * 0.30), 2.0) /
                               (2.0 * gravity);
  close(result.flow, {0.01});
  close(result.head, {50.0, 50.0 - expected_loss}, 2e-8);
  close(result.head_loss, {expected_loss}, 2e-8);
  close(result.friction_factor, {friction}, 2e-10);
  close(result.pressure,
        {50.0 * density * gravity,
         (50.0 - expected_loss - 5.0) * density * gravity},
        2e-4);
  expect(result.energy_residual.abs().max().item<double>() < 1e-9,
         "pipe energy equation did not close");
  expect(result.balance.select(-1, 1).abs().item<double>() < 1e-11,
         "junction continuity did not close");
  expect(result.converged.item<bool>(), "one-pipe solve did not converge");

  fluid::Options hazen;
  hazen.loss = fluid::LossModel::HazenWilliams;
  auto hazen_net = one_pipe();
  hazen_net.pipes.roughness = pluss::series::make({130.0});
  const auto hazen_result = fluid::steady(fluid::network(hazen_net), hazen);
  const double hazen_loss = 10.67 * 1000.0 * std::pow(0.01, 1.852) /
                            (std::pow(130.0, 1.852) * std::pow(0.30, 4.8704));
  close(hazen_result.head_loss, {hazen_loss}, 2e-9);
  close(hazen_result.friction_factor, {0.0});

  auto regimes = one_pipe();
  regimes.nodes.demand =
      pluss::series::make(std::vector<double>{0.0, 1e-5, 0.0, 7e-4}, {2, 2});
  regimes.nodes.head = pluss::series::make({50.0, 49.0});
  const auto regime_result = fluid::steady(fluid::network(regimes));
  const double re_laminar = 4.0e-5 / (pi * 0.30 * viscosity);
  close(regime_result.friction_factor.select(-1, 0),
        {64.0 / re_laminar, epanet_transition(7e-4, 0.30, 1e-4, viscosity)},
        2e-8);

  fluid::Net loop;
  loop.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction,
                     fluid::NodeKind::Junction};
  loop.nodes.elevation = pluss::series::make({0.0, 2.0, 3.0});
  loop.nodes.demand = pluss::series::make({0.0, 0.006, 0.004});
  loop.nodes.head = pluss::series::make({60.0, 58.0, 57.0});
  loop.pipes.from = {0, 1, 0};
  loop.pipes.to = {1, 2, 2};
  loop.pipes.length = pluss::series::make({500.0, 400.0, 700.0});
  loop.pipes.diameter = pluss::series::make({0.25, 0.20, 0.22});
  loop.pipes.roughness = pluss::series::make({1e-4, 1e-4, 1e-4});
  const auto loop_result = fluid::steady(fluid::network(loop));
  expect(loop_result.balance.slice(-1, 1, 3).abs().max().item<double>() < 1e-10,
         "looped network continuity did not close");
  expect(loop_result.energy_residual.abs().max().item<double>() < 1e-8,
         "looped network energy did not close");

  auto batch = one_pipe();
  batch.nodes.demand =
      pluss::series::make(std::vector<double>{0.0, 0.008, 0.0, 0.012}, {2, 2});
  batch.pipes.length =
      pluss::series::make(std::vector<double>{800.0, 1200.0}, {2, 1});
  const auto batch_result = fluid::steady(fluid::network(batch));
  expect(batch_result.head.sizes() == std::vector<std::int64_t>({2, 2}) &&
             batch_result.flow.sizes() == std::vector<std::int64_t>({2, 1}),
         "batch axes were not preserved");
  close(batch_result.flow, {0.008, 0.012});

  auto float_net = one_pipe();
  float_net.nodes.elevation =
      float_net.nodes.elevation.to(pluss::ScalarType::Float32);
  float_net.nodes.demand =
      float_net.nodes.demand.to(pluss::ScalarType::Float32);
  float_net.nodes.head = float_net.nodes.head.to(pluss::ScalarType::Float32);
  float_net.pipes.length =
      float_net.pipes.length.to(pluss::ScalarType::Float32);
  float_net.pipes.diameter =
      float_net.pipes.diameter.to(pluss::ScalarType::Float32);
  float_net.pipes.roughness =
      float_net.pipes.roughness.to(pluss::ScalarType::Float32);
  float_net.pipes.minor = float_net.pipes.minor.to(pluss::ScalarType::Float32);
  fluid::Options float_options;
  float_options.head_tolerance = 2e-5;
  float_options.flow_tolerance = 2e-7;
  const auto float_result =
      fluid::steady(fluid::network(float_net), float_options);
  expect(float_result.head.scalar_type() == pluss::ScalarType::Float32,
         "Float32 was not retained");

  auto tracked_net = one_pipe();
  tracked_net.nodes.demand.tracks_slope_(true);
  const auto tracked = fluid::steady(fluid::network(tracked_net));
  tracked.head.select(-1, 1).reverse();
  expect(
      tracked_net.nodes.demand.has_sensitivity() &&
          tracked_net.nodes.demand.sensitivity().isfinite().all().item<bool>(),
      "hydraulic reverse sensitivity is absent or non-finite");
  const auto pipe_loss = [&](double q) {
    const double f = swamee_jain(q, 0.30, 1e-4, viscosity);
    const double velocity = 4.0 * q / (pi * 0.30 * 0.30);
    return f * (1000.0 / 0.30) * velocity * std::abs(velocity) /
           (2.0 * gravity);
  };
  const double epsilon = 1e-7;
  const double demand_slope =
      -(pipe_loss(0.01 + epsilon) - pipe_loss(0.01 - epsilon)) /
      (2.0 * epsilon);
  close(tracked_net.nodes.demand.sensitivity(), {0.0, demand_slope}, 2e-5);

  fluid::Options inspect_failure;
  inspect_failure.max_iterations = 1;
  inspect_failure.fail_on_nonconvergence = false;
  const auto unfinished = fluid::steady(fluid::network(loop), inspect_failure);
  expect(!unfinished.converged.item<bool>() &&
             unfinished.energy_residual.abs().max().item<double>() >
                 inspect_failure.head_tolerance,
         "non-throwing iteration limit did not expose its residual");
  inspect_failure.fail_on_nonconvergence = true;
  expect_runtime_error(
      [&] { (void)fluid::steady(fluid::network(loop), inspect_failure); },
      "nonconvergence did not raise under the default failure policy");

  auto pump_net = one_pump();
  const auto pump_result = fluid::steady(fluid::network(pump_net));
  expect(fluid::network(pump_net).pipes() == 0 &&
             fluid::network(pump_net).pumps() == 1 &&
             fluid::network(pump_net).links() == 1,
         "pump-only topology counts are wrong");
  close(pump_result.flow, {0.01});
  close(pump_result.head_loss, {-29.0});
  close(pump_result.head, {10.0, 39.0});
  close(pump_result.velocity, {0.0});

  pump_net.pumps.speed =
      pluss::series::make(std::vector<double>{0.5, 1.0}, {2, 1});
  const auto speed_sweep = fluid::steady(fluid::network(pump_net));
  close(speed_sweep.head.select(-1, 1), {16.5, 39.0}, 2e-8);

  auto tracked_pump = one_pump();
  tracked_pump.pumps.speed.tracks_slope_(true);
  fluid::steady(fluid::network(tracked_pump)).head.select(-1, 1).reverse();
  close(tracked_pump.pumps.speed.sensitivity(), {60.0}, 2e-6);

  fluid::Net throttle;
  throttle.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  throttle.nodes.elevation = pluss::series::make({0.0, 0.0});
  throttle.nodes.demand = pluss::series::make({0.0, 0.01});
  throttle.nodes.head = pluss::series::make({20.0, 19.0});
  throttle.valves.kind = {fluid::ValveKind::Throttle};
  throttle.valves.from = {0};
  throttle.valves.to = {1};
  throttle.valves.diameter = pluss::series::make({0.2});
  throttle.valves.minor = pluss::series::make({8.0});
  throttle.valves.setting = pluss::series::make({0.0});
  const auto throttle_result = fluid::steady(fluid::network(throttle));
  const double throttle_loss =
      8.0 * std::pow(4.0 * 0.01 / (pi * 0.2 * 0.2), 2.0) / (2.0 * gravity);
  close(throttle_result.head_loss, {throttle_loss}, 2e-9);

  auto reducing = throttle;
  reducing.nodes.elevation = pluss::series::make({0.0, 5.0});
  reducing.nodes.head = pluss::series::make({50.0, 25.0});
  reducing.valves.kind = {fluid::ValveKind::Reduce};
  reducing.valves.setting = pluss::series::make({20.0});
  const auto reducing_result = fluid::steady(fluid::network(reducing));
  close(reducing_result.head, {50.0, 25.0});
  close(reducing_result.flow, {0.01});
  close(reducing_result.head_loss, {25.0});

  fluid::Net sustaining;
  sustaining.nodes.kind = {fluid::NodeKind::Reservoir,
                           fluid::NodeKind::Junction,
                           fluid::NodeKind::Reservoir};
  sustaining.nodes.elevation = pluss::series::make({0.0, 5.0, 0.0});
  sustaining.nodes.demand = pluss::series::make({0.0, 0.0, 0.0});
  sustaining.nodes.head = pluss::series::make({40.0, 25.0, 0.0});
  sustaining.pipes.from = {0};
  sustaining.pipes.to = {1};
  sustaining.pipes.length = pluss::series::make({100.0});
  sustaining.pipes.diameter = pluss::series::make({0.3});
  sustaining.pipes.roughness = pluss::series::make({1e-4});
  sustaining.valves.kind = {fluid::ValveKind::Sustain};
  sustaining.valves.from = {1};
  sustaining.valves.to = {2};
  sustaining.valves.diameter = pluss::series::make({0.2});
  sustaining.valves.minor = pluss::series::make({0.0});
  sustaining.valves.setting = pluss::series::make({20.0});
  const auto sustaining_result = fluid::steady(fluid::network(sustaining));
  close(sustaining_result.head, {40.0, 25.0, 0.0}, 2e-8);
  expect(std::abs(sustaining_result.flow.select(-1, 0).item<double>() -
                  sustaining_result.flow.select(-1, 1).item<double>()) < 1e-10,
         "sustaining-valve continuity did not close");
  close(sustaining_result.head_loss.select(-1, 1), {25.0}, 2e-8);

  auto controlled = loop;
  controlled.valves.kind = {fluid::ValveKind::Flow};
  controlled.valves.from = {0};
  controlled.valves.to = {2};
  controlled.valves.diameter = pluss::series::make({0.2});
  controlled.valves.minor = pluss::series::make({0.0});
  controlled.valves.setting = pluss::series::make({0.001});
  const auto flow_control = fluid::steady(fluid::network(controlled));
  close(flow_control.flow.select(-1, 3), {0.001}, 2e-10);
  expect(flow_control.energy_residual.abs().max().item<double>() < 1e-8,
         "flow-control constraint did not close");

  // A pressure-driven demand far above what the pipe can deliver makes the undamped Newton step cycle between
  // two far-apart heads; the relaxed step converges to the Wagner-law solution.
  {
    auto starved = one_pipe(0.4);
    fluid::Options pressure_driven;
    pressure_driven.demand_mode = fluid::DemandMode::Pressure;
    pressure_driven.full_pressure = 15.0;
    const auto starved_result = fluid::steady(fluid::network(starved), pressure_driven);
    const double head = starved_result.head.select(-1, 1).item<double>();
    const double delivered = starved_result.demand.select(-1, 1).item<double>();
    expect(starved_result.converged.item<bool>() && head > 5.0 && head < 20.0 &&
               std::abs(delivered - 0.4 * std::sqrt((head - 5.0) / 15.0)) < 1e-9,
           "starved pressure-driven demand did not converge to the Wagner solution");
  }

  // A pump closed by a control while the only tank drains to its minimum level leaves the demand junction cut off
  // from every source; that is reported as a disconnected system, not as a bare linear-algebra failure.
  expect_runtime_error(
      [] {
        fluid::Net net;
        net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction,
                          fluid::NodeKind::Tank};
        net.nodes.elevation = pluss::series::make({0.0, 12.0, 20.0});
        net.nodes.demand = pluss::series::make({0.0, 0.03, 0.0});
        net.nodes.head = pluss::series::make({10.0, 30.0, 23.0});
        net.tanks.area = pluss::series::make({200.0});
        net.tanks.min_level = pluss::series::make({0.5});
        net.tanks.max_level = pluss::series::make({6.0});
        net.tanks.overflow = {false};
        net.pumps.from = {0};
        net.pumps.to = {1};
        net.pumps.shutoff_head = pluss::series::make({60.0});
        net.pumps.resistance = pluss::series::make({1500.0});
        net.pumps.exponent = pluss::series::make({2.0});
        net.pumps.speed = pluss::series::make({1.0});
        net.pipes.from = {1};
        net.pipes.to = {2};
        net.pipes.length = pluss::series::make({300.0});
        net.pipes.diameter = pluss::series::make({0.20});
        net.pipes.roughness = pluss::series::make({1e-4});
        net.pipes.minor = pluss::series::make({0.0});
        fluid::Schedule schedule;
        schedule.duration = 24.0 * 3600.0;
        schedule.step = 1800.0;
        schedule.time = {{3600.0, {fluid::Device::Pump, 0, fluid::Act::Close, 0.0}}};
        (void)fluid::run(fluid::network(net), schedule);
      },
      "a demand cut off from every source was not reported");

  // A valve cannot raise the head across it. A sustaining setting well below the natural upstream
  // pressure could only be met by draining the junction through the valve and lifting the water
  // downstream of it above the reservoir, which is refused.
  expect_runtime_error(
      [] {
        fluid::Net parallel;
        parallel.nodes.kind = {fluid::NodeKind::Reservoir,
                               fluid::NodeKind::Junction,
                               fluid::NodeKind::Junction};
        parallel.nodes.elevation = pluss::series::make({0.0, 10.0, 5.0});
        parallel.nodes.demand = pluss::series::make({0.0, 0.02, 0.02});
        parallel.nodes.head = pluss::series::make({80.0, 55.0, 55.0});
        parallel.pipes.from = {0, 0};
        parallel.pipes.to = {1, 2};
        parallel.pipes.length = pluss::series::make({600.0, 900.0});
        parallel.pipes.diameter = pluss::series::make({0.20, 0.15});
        parallel.pipes.roughness = pluss::series::make({1e-4, 1e-4});
        parallel.valves.kind = {fluid::ValveKind::Sustain};
        parallel.valves.from = {1};
        parallel.valves.to = {2};
        parallel.valves.diameter = pluss::series::make({0.2});
        parallel.valves.minor = pluss::series::make({0.0});
        parallel.valves.setting = pluss::series::make({50.0});
        (void)fluid::steady(fluid::network(parallel));
      },
      "an energy-adding sustaining valve was accepted");

  auto float_pump = one_pump();
  float_pump.nodes.elevation =
      float_pump.nodes.elevation.to(pluss::ScalarType::Float32);
  float_pump.nodes.demand =
      float_pump.nodes.demand.to(pluss::ScalarType::Float32);
  float_pump.nodes.head = float_pump.nodes.head.to(pluss::ScalarType::Float32);
  float_pump.pumps.shutoff_head =
      float_pump.pumps.shutoff_head.to(pluss::ScalarType::Float32);
  float_pump.pumps.resistance =
      float_pump.pumps.resistance.to(pluss::ScalarType::Float32);
  float_pump.pumps.exponent =
      float_pump.pumps.exponent.to(pluss::ScalarType::Float32);
  float_pump.pumps.speed =
      float_pump.pumps.speed.to(pluss::ScalarType::Float32);
  const auto float_pump_result =
      fluid::steady(fluid::network(float_pump), float_options);
  expect(float_pump_result.head.scalar_type() == pluss::ScalarType::Float32 &&
             float_pump_result.flow.scalar_type() == pluss::ScalarType::Float32,
         "pump solve did not retain Float32");

  expect_runtime_error(
      [] {
        auto reverse = one_pump();
        reverse.nodes.demand = pluss::series::make({0.0, -0.01});
        (void)fluid::steady(fluid::network(reverse));
      },
      "reverse pump flow was accepted as turbine operation");

  auto pressure_net = one_pipe();
  pressure_net.nodes.elevation = pluss::series::make({0.0, 0.0});
  pressure_net.nodes.head = pluss::series::make({10.0, 9.0});
  pressure_net.pipes.length = pluss::series::make({1.0});
  pressure_net.pipes.diameter = pluss::series::make({1.0});
  fluid::Options pressure_options;
  pressure_options.demand_mode = fluid::DemandMode::Pressure;
  pressure_options.min_pressure = 0.0;
  pressure_options.full_pressure = 20.0;
  pressure_options.demand_exponent = 0.5;
  const auto pressure_result =
      fluid::steady(fluid::network(pressure_net), pressure_options);
  const double pressure_head =
      pressure_result.head.select(-1, 1).item<double>();
  const double delivered = 0.01 * std::sqrt(pressure_head / 20.0);
  close(pressure_result.demand, {0.0, delivered}, 2e-10);
  close(pressure_result.flow, {delivered}, 2e-10);

  auto emitter_net = pressure_net;
  emitter_net.nodes.demand = pluss::series::make({0.0, 0.0});
  emitter_net.nodes.emitter = pluss::series::make({0.0, 0.001});
  const auto emitter_result = fluid::steady(fluid::network(emitter_net));
  const double emitter_head = emitter_result.head.select(-1, 1).item<double>();
  const double emitter_flow = 0.001 * std::sqrt(emitter_head);
  close(emitter_result.emitter, {0.0, emitter_flow}, 2e-10);
  close(emitter_result.flow, {emitter_flow}, 2e-10);

  const auto tank_compiled = fluid::network(tank_pipe());
  expect(tank_compiled.tanks() == 1 && tank_compiled.reservoirs() == 0,
         "tank was not compiled as a dynamic fixed-grade node");
  fluid::Schedule tank_schedule;
  tank_schedule.duration = 100.0;
  tank_schedule.step = 100.0;
  tank_schedule.pattern_step = 50.0;
  tank_schedule.demand =
      pluss::series::make(std::vector<double>{1.0, 1.0, 1.0, 2.0}, {2, 2});
  const auto tank_run = fluid::run(tank_compiled, tank_schedule);
  close(tank_run.time, {0.0, 50.0, 100.0});
  close(tank_run.tank_level, {10.0, 9.95, 9.85}, 2e-9);
  close(tank_run.demand.select(-1, 1), {0.01, 0.02, 0.01}, 2e-10);

  auto tracked_tank = tank_pipe();
  tracked_tank.nodes.demand.tracks_slope_(true);
  fluid::Schedule tracked_schedule;
  tracked_schedule.duration = 100.0;
  tracked_schedule.step = 50.0;
  tracked_schedule.pattern_step = 50.0;
  const auto tracked_run =
      fluid::run(fluid::network(tracked_tank), tracked_schedule);
  tracked_run.tank_level.select(0, tracked_run.tank_level.size(0) - 1)
      .select(-1, 0)
      .reverse();
  close(tracked_tank.nodes.demand.sensitivity(), {0.0, -10.0}, 2e-7);

  fluid::Schedule pump_schedule;
  pump_schedule.duration = 100.0;
  pump_schedule.step = 100.0;
  pump_schedule.pattern_step = 100.0;
  pump_schedule.time = {{50.0, {fluid::Device::Pump, 0, fluid::Act::Set, 0.5}}};
  const auto pump_run = fluid::run(fluid::network(one_pump()), pump_schedule);
  close(pump_run.time, {0.0, 50.0, 100.0});
  close(pump_run.head.select(-1, 1), {39.0, 16.5, 16.5}, 2e-8);

  fluid::Schedule throttle_schedule;
  throttle_schedule.duration = 10.0;
  throttle_schedule.step = 10.0;
  throttle_schedule.pattern_step = 10.0;
  throttle_schedule.time = {
      {5.0, {fluid::Device::Valve, 0, fluid::Act::Set, 32.0}}};
  const auto throttle_run =
      fluid::run(fluid::network(throttle), throttle_schedule);
  close(throttle_run.head_loss,
        {throttle_loss, 4.0 * throttle_loss, 4.0 * throttle_loss}, 2e-9);

  fluid::Net tank_pump;
  tank_pump.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction,
                          fluid::NodeKind::Tank};
  tank_pump.nodes.elevation = pluss::series::make({0.0, 0.0, 0.0});
  tank_pump.nodes.demand = pluss::series::make({0.0, 0.0, 0.0});
  tank_pump.nodes.head = pluss::series::make({0.0, 15.0, 10.0});
  tank_pump.tanks.area = pluss::series::make({1.0});
  tank_pump.tanks.min_level = pluss::series::make({0.0});
  tank_pump.tanks.max_level = pluss::series::make({20.0});
  tank_pump.tanks.overflow = {false};
  tank_pump.pumps.from = {0};
  tank_pump.pumps.to = {1};
  tank_pump.pumps.shutoff_head = pluss::series::make({30.0});
  tank_pump.pumps.resistance = pluss::series::make({1000.0});
  tank_pump.pumps.exponent = pluss::series::make({2.0});
  tank_pump.pipes.from = {1};
  tank_pump.pipes.to = {2};
  tank_pump.pipes.length = pluss::series::make({100.0});
  tank_pump.pipes.diameter = pluss::series::make({0.3});
  tank_pump.pipes.roughness = pluss::series::make({1e-4});
  fluid::Schedule level_schedule;
  level_schedule.duration = 20.0;
  level_schedule.step = 20.0;
  level_schedule.pattern_step = 20.0;
  level_schedule.state = {{2,
                           fluid::Sense::Above,
                           10.2,
                           {fluid::Device::Pump, 0, fluid::Act::Close, 0.0}}};
  const auto level_run = fluid::run(fluid::network(tank_pump), level_schedule);
  expect(level_run.time.size(0) == 3,
         "tank threshold did not introduce one exact hydraulic event; got " +
             std::to_string(level_run.time.size(0)) + " samples");
  close(level_run.tank_level, {10.0, 10.2, 10.2}, 2e-8);
  close(level_run.open.select(-1, 1), {1.0, 0.0, 0.0});
  close(level_run.flow.select(-1, 0).slice(0, 1, 3), {0.0, 0.0}, 2e-10);

  auto batched_tank = tank_pipe();
  batched_tank.nodes.demand =
      pluss::series::make(std::vector<double>{0.0, 0.01, 0.0, 0.02}, {2, 2});
  fluid::Schedule batch_schedule;
  batch_schedule.duration = 100.0;
  batch_schedule.step = 100.0;
  batch_schedule.pattern_step = 100.0;
  const auto batch_run =
      fluid::run(fluid::network(batched_tank), batch_schedule);
  expect(batch_run.tank_level.sizes() == std::vector<std::int64_t>({2, 2, 1}),
         "extended-period batch axes were not preserved");
  close(batch_run.tank_level.select(0, 1), {9.9, 9.8}, 2e-9);

  auto float_tank = tank_pipe();
  float_tank.nodes.elevation =
      float_tank.nodes.elevation.to(pluss::ScalarType::Float32);
  float_tank.nodes.demand =
      float_tank.nodes.demand.to(pluss::ScalarType::Float32);
  float_tank.nodes.head = float_tank.nodes.head.to(pluss::ScalarType::Float32);
  float_tank.tanks.area = float_tank.tanks.area.to(pluss::ScalarType::Float32);
  float_tank.tanks.min_level =
      float_tank.tanks.min_level.to(pluss::ScalarType::Float32);
  float_tank.tanks.max_level =
      float_tank.tanks.max_level.to(pluss::ScalarType::Float32);
  float_tank.pipes.length =
      float_tank.pipes.length.to(pluss::ScalarType::Float32);
  float_tank.pipes.diameter =
      float_tank.pipes.diameter.to(pluss::ScalarType::Float32);
  float_tank.pipes.roughness =
      float_tank.pipes.roughness.to(pluss::ScalarType::Float32);
  fluid::Schedule float_schedule;
  float_schedule.duration = 10.0;
  float_schedule.step = 10.0;
  float_schedule.pattern_step = 10.0;
  const auto float_run =
      fluid::run(fluid::network(float_tank), float_schedule, float_options);
  expect(float_run.head.scalar_type() == pluss::ScalarType::Float32 &&
             float_run.tank_level.scalar_type() == pluss::ScalarType::Float32,
         "extended-period run did not retain Float32");

  fluid::Net overflow;
  overflow.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction,
                         fluid::NodeKind::Tank};
  overflow.nodes.elevation = pluss::series::make({0.0, 0.0, 0.0});
  overflow.nodes.demand = pluss::series::make({0.0, 0.0, 0.0});
  overflow.nodes.head = pluss::series::make({30.0, 25.0, 20.0});
  overflow.tanks.area = pluss::series::make({2.0});
  overflow.tanks.min_level = pluss::series::make({0.0});
  overflow.tanks.max_level = pluss::series::make({20.0});
  overflow.tanks.overflow = {true};
  overflow.pipes.from = {0, 1};
  overflow.pipes.to = {1, 2};
  overflow.pipes.length = pluss::series::make({100.0, 100.0});
  overflow.pipes.diameter = pluss::series::make({0.3, 0.3});
  overflow.pipes.roughness = pluss::series::make({1e-4, 1e-4});
  fluid::Schedule overflow_schedule;
  overflow_schedule.duration = 10.0;
  overflow_schedule.step = 10.0;
  overflow_schedule.pattern_step = 10.0;
  const auto overflow_run =
      fluid::run(fluid::network(overflow), overflow_schedule);
  close(overflow_run.tank_level, {20.0, 20.0});
  expect(overflow_run.spill.gt(0.0).all().item<bool>(),
         "overflowing tank did not report conserved spill discharge");

  auto bounded = overflow;
  bounded.nodes.head = pluss::series::make({30.0, 25.0, 19.9});
  bounded.tanks.overflow = {false};
  const auto bounded_run =
      fluid::run(fluid::network(bounded), overflow_schedule);
  expect(bounded_run.time.size(0) == 3,
         "tank maximum did not shorten the hydraulic step");
  close(bounded_run.tank_level, {19.9, 20.0, 20.0}, 2e-8);
  close(bounded_run.spill, {0.0, 0.0, 0.0});
  close(bounded_run.open.select(-1, 1), {1.0, 0.0, 0.0});

  expect_error(
      [&] {
        auto bad = tank_schedule;
        bad.demand = pluss::series::ones({2, 1});
        (void)fluid::run(tank_compiled, bad);
      },
      "wrong-width demand pattern was accepted");
  expect_error(
      [] {
        auto bad = tank_pipe();
        bad.tanks.max_level = pluss::series::make({0.0});
        (void)fluid::network(bad);
      },
      "inverted tank bounds were accepted");

  expect_error(
      [] {
        auto bad = one_pipe();
        bad.pipes.to = {0};
        (void)fluid::network(bad);
      },
      "self-pipe was accepted");
  expect_error(
      [] {
        auto bad = one_pipe();
        bad.nodes.kind = {fluid::NodeKind::Junction, fluid::NodeKind::Junction};
        (void)fluid::network(bad);
      },
      "reservoir-free component was accepted");
  expect_error(
      [] {
        auto bad = one_pipe();
        bad.pipes.diameter = pluss::series::make({0.0});
        (void)fluid::network(bad);
      },
      "zero diameter was accepted");
  expect_error(
      [] {
        fluid::Options options;
        options.loss = fluid::LossModel::HazenWilliams;
        auto bad = one_pipe();
        bad.pipes.roughness = pluss::series::make({0.0});
        (void)fluid::steady(fluid::network(bad), options);
      },
      "zero Hazen-Williams coefficient was accepted");

  // E7d: a Courant-one front crosses one four-cell pipe without numerical
  // smearing.  The pipe volume and imposed demand determine the travel time.
  const double pipe_volume = pi * 0.30 * 0.30 * 1000.0 / 4.0;
  const double travel_time = pipe_volume / 0.01;
  fluid::Schedule transport_schedule;
  transport_schedule.duration = travel_time;
  transport_schedule.step = travel_time;
  transport_schedule.pattern_step = travel_time;
  fluid::Quality constituent;
  constituent.initial = pluss::series::make({1.0, 0.0});
  constituent.source = pluss::series::make({1.0, 0.0});
  fluid::RouteOptions transport_options;
  transport_options.cells = 4;
  transport_options.courant = 1.0;
  transport_options.step = travel_time / 4.0;
  const auto transported = fluid::transport(
      fluid::network(one_pipe()), transport_schedule, constituent,
      transport_options);
  close(transported.node.select(0, 0), {1.0, 0.0});
  close(transported.node.select(0, 1), {1.0, 1.0}, 2e-8);
  close(transported.link.select(0, 1), {1.0}, 2e-8);
  expect(transported.mass_error.abs().max().item<double>() < 2e-8,
         "constituent transport did not conserve boundary-adjusted mass");

  // Complete mixing is flow-weighted, not an arithmetic average of links.
  fluid::Net mixer;
  mixer.nodes.kind = {fluid::NodeKind::Reservoir,
                      fluid::NodeKind::Reservoir,
                      fluid::NodeKind::Junction};
  mixer.nodes.elevation = pluss::series::make({0.0, 0.0, 0.0});
  mixer.nodes.demand = pluss::series::make({0.0, 0.0, 0.02});
  mixer.nodes.head = pluss::series::make({50.0, 50.0, 49.0});
  mixer.pipes.from = {0, 1};
  mixer.pipes.to = {2, 2};
  mixer.pipes.length = pluss::series::make({1000.0, 1000.0});
  mixer.pipes.diameter = pluss::series::make({0.30, 0.30});
  mixer.pipes.roughness = pluss::series::make({1e-4, 1e-4});
  fluid::Quality mixed_quality;
  mixed_quality.initial = pluss::series::make({1.0, 3.0, 0.0});
  mixed_quality.source = pluss::series::make({1.0, 3.0, 0.0});
  const auto mixed = fluid::transport(fluid::network(mixer),
                                      transport_schedule, mixed_quality,
                                      transport_options);
  expect(std::abs(mixed.node.select(0, 1).select(-1, 2).item<double>() - 2.0) <
             2e-8,
         "equal tributaries did not produce their flow-weighted mixture");

  // First-order kinetics use the exact exponential over every quality step.
  fluid::Quality decaying;
  decaying.initial = pluss::series::make({1.0, 1.0});
  decaying.pipe_rate = pluss::series::make({-0.1});
  fluid::Schedule decay_schedule;
  decay_schedule.duration = 10.0;
  decay_schedule.step = 10.0;
  decay_schedule.pattern_step = 10.0;
  fluid::RouteOptions decay_options;
  decay_options.cells = 3;
  decay_options.step = 2.0;
  const auto decay = fluid::transport(fluid::network(one_pipe(0.0)),
                                      decay_schedule, decaying, decay_options);
  expect(std::abs(decay.link.select(0, 1).select(-1, 0).item<double>() -
                  std::exp(-1.0)) <
             2e-10,
         "pipe first-order decay did not match the exact exponential");
  expect(decay.mass_error.abs().max().item<double>() < 2e-9,
         "pipe reaction was not reconciled in the mass ledger");

  // Water age owns its zero boundary and one-second-per-second growth law.
  fluid::Quality water_age;
  water_age.mode = fluid::QualityMode::Age;
  const auto aged = fluid::transport(fluid::network(one_pipe()),
                                     transport_schedule, water_age,
                                     transport_options);
  const double cell_time = travel_time / 4.0;
  expect(std::abs(aged.node.select(0, 1).select(-1, 1).item<double>() -
                  4.0 * cell_time) < 2e-7,
         "water age did not advance and reset across the pipe boundary");
  expect(aged.node.min().item<double>() >= 0.0,
         "water age produced a negative value");

  // A fully mixed draining tank keeps concentration under pure withdrawal;
  // its independent reaction field changes concentration exponentially.
  fluid::Quality tank_decay;
  tank_decay.initial = pluss::series::make({2.0, 0.0});
  tank_decay.tank_rate = pluss::series::make({-0.1});
  const auto tank_quality_run = fluid::transport(
      fluid::network(tank_pipe()), decay_schedule, tank_decay, decay_options);
  expect(std::abs(tank_quality_run.tank.select(0, 1).select(-1, 0).item<double>() -
                  2.0 * std::exp(-1.0)) < 2e-9,
         "fully mixed tank decay did not match the exact exponential");
  expect(tank_quality_run.mass_error.abs().max().item<double>() < 2e-8,
         "tank reaction was not reconciled in the mass ledger");

  // The source remains on the reverse graph through routing and mixing.
  auto tracked_source = pluss::series::make({1.0, 0.0});
  tracked_source.tracks_slope_(true);
  fluid::Quality tracked_quality;
  tracked_quality.initial = pluss::series::make({0.0, 0.0});
  tracked_quality.source = tracked_source;
  const auto tracked_transport = fluid::transport(
      fluid::network(one_pipe()), transport_schedule, tracked_quality,
      transport_options);
  tracked_transport.node.select(0, 1).select(-1, 1).reverse();
  close(tracked_source.sensitivity(), {1.0, 0.0}, 2e-7);

  // Pumps and valves have no transport storage.  Their concentration is an
  // algebraic pass-through in the same node-mixing system, without a fake
  // residence volume or a one-step lag.
  fluid::Quality pump_quality;
  pump_quality.initial = pluss::series::make({2.0, 0.0});
  pump_quality.source = pluss::series::make({2.0, 0.0});
  fluid::Schedule pump_quality_schedule;
  pump_quality_schedule.duration = 1.0;
  pump_quality_schedule.step = 1.0;
  pump_quality_schedule.pattern_step = 1.0;
  const auto pump_transport = fluid::transport(
      fluid::network(one_pump()), pump_quality_schedule, pump_quality,
      decay_options);
  close(pump_transport.node.select(0, 0), {2.0, 2.0}, 2e-9);
  close(pump_transport.link.select(0, 0), {2.0}, 2e-9);

  // Declared endpoint order does not constrain transport direction.
  fluid::Net reverse_pipe;
  reverse_pipe.nodes.kind = {fluid::NodeKind::Junction,
                             fluid::NodeKind::Reservoir};
  reverse_pipe.nodes.elevation = pluss::series::make({5.0, 0.0});
  reverse_pipe.nodes.demand = pluss::series::make({0.01, 0.0});
  reverse_pipe.nodes.head = pluss::series::make({49.0, 50.0});
  reverse_pipe.pipes.from = {0};
  reverse_pipe.pipes.to = {1};
  reverse_pipe.pipes.length = pluss::series::make({1000.0});
  reverse_pipe.pipes.diameter = pluss::series::make({0.30});
  reverse_pipe.pipes.roughness = pluss::series::make({1e-4});
  fluid::Quality reverse_quality;
  reverse_quality.initial = pluss::series::make({0.0, 4.0});
  reverse_quality.source = pluss::series::make({0.0, 4.0});
  const auto reverse_transport = fluid::transport(
      fluid::network(reverse_pipe), transport_schedule, reverse_quality,
      transport_options);
  expect(std::abs(reverse_transport.node.select(0, 1)
                      .select(-1, 0)
                      .item<double>() -
                  4.0) < 2e-8,
         "reverse flow did not reverse the transport stencil");

  // Scenario axes remain batched through hydraulics, routing, and mixing.
  auto batched_pipe = one_pipe();
  batched_pipe.nodes.elevation =
      pluss::series::make(std::vector<double>{0.0, 5.0, 0.0, 5.0}, {2, 2});
  batched_pipe.nodes.demand =
      pluss::series::make(std::vector<double>{0.0, 0.01, 0.0, 0.02}, {2, 2});
  batched_pipe.nodes.head =
      pluss::series::make(std::vector<double>{50.0, 49.0, 50.0, 49.0},
                          {2, 2});
  batched_pipe.pipes.length =
      pluss::series::make(std::vector<double>{1000.0, 1000.0}, {2, 1});
  batched_pipe.pipes.diameter =
      pluss::series::make(std::vector<double>{0.30, 0.30}, {2, 1});
  batched_pipe.pipes.roughness =
      pluss::series::make(std::vector<double>{1e-4, 1e-4}, {2, 1});
  batched_pipe.pipes.minor =
      pluss::series::make(std::vector<double>{0.0, 0.0}, {2, 1});
  fluid::Quality batched_quality;
  batched_quality.initial =
      pluss::series::make(std::vector<double>{1.0, 0.0, 2.0, 0.0}, {2, 2});
  batched_quality.source = batched_quality.initial;
  const auto batched_transport = fluid::transport(
      fluid::network(batched_pipe), transport_schedule, batched_quality,
      transport_options);
  expect(batched_transport.node.sizes() ==
             std::vector<std::int64_t>({2, 2, 2}),
         "transport did not preserve the scenario axis");
  close(batched_transport.node.select(0, 1).select(-1, 1),
        {0.63671875, 2.0}, 3e-8);
  expect(batched_transport.mass_error.abs().max().item<double>() < 4e-8,
         "batched transport did not conserve mass");

  // Float32 is retained end-to-end; no hidden Float64 quality state is made.
  auto float_pipe = one_pipe();
  float_pipe.nodes.elevation =
      float_pipe.nodes.elevation.to(pluss::ScalarType::Float32);
  float_pipe.nodes.demand =
      float_pipe.nodes.demand.to(pluss::ScalarType::Float32);
  float_pipe.nodes.head = float_pipe.nodes.head.to(pluss::ScalarType::Float32);
  float_pipe.pipes.length =
      float_pipe.pipes.length.to(pluss::ScalarType::Float32);
  float_pipe.pipes.diameter =
      float_pipe.pipes.diameter.to(pluss::ScalarType::Float32);
  float_pipe.pipes.roughness =
      float_pipe.pipes.roughness.to(pluss::ScalarType::Float32);
  float_pipe.pipes.minor =
      float_pipe.pipes.minor.to(pluss::ScalarType::Float32);
  fluid::Quality float_quality;
  float_quality.initial =
      pluss::series::make({1.0, 0.0}).to(pluss::ScalarType::Float32);
  float_quality.source = float_quality.initial;
  const auto float_transport = fluid::transport(
      fluid::network(float_pipe), transport_schedule, float_quality,
      transport_options, float_options);
  expect(float_transport.node.scalar_type() == pluss::ScalarType::Float32 &&
             float_transport.link.scalar_type() == pluss::ScalarType::Float32,
         "quality transport did not retain Float32");

  // E7e: heat uses the same conservative route owner.  With stagnant fluid,
  // a lumped pipe UA has the exact constant-property solution
  // Ta + (T0 - Ta) exp[-UA t/(rho cp V)].
  constexpr double cp = 4180.0;
  constexpr double thermal_rate = 0.1;
  fluid::Heat stagnant_heat;
  stagnant_heat.initial = pluss::series::make({350.0, 350.0});
  stagnant_heat.ambient = pluss::series::make({300.0});
  stagnant_heat.ua = pluss::series::make(
      {density * cp * pipe_volume * thermal_rate});
  stagnant_heat.cp = cp;
  const auto cooled = fluid::heat(fluid::network(one_pipe(0.0)),
                                  decay_schedule, stagnant_heat,
                                  decay_options);
  const double cooled_temperature = 300.0 + 50.0 * std::exp(-1.0);
  close(cooled.link.select(0, 1), {cooled_temperature}, 2e-8);
  expect(cooled.energy_error.abs().max().item<double>() < 5e-2,
         "thermal reaction was not reconciled in the energy ledger");

  // A moving plug experiences the same exact exponential over its residence
  // time.  The Courant-one case separates heat-transfer error from advection
  // smearing and verifies that the source boundary remains prescribed.
  fluid::Heat moving_heat;
  moving_heat.initial = pluss::series::make({300.0, 300.0});
  moving_heat.source = pluss::series::make({340.0, 300.0});
  moving_heat.ambient = pluss::series::make({300.0});
  moving_heat.ua =
      pluss::series::make({density * cp * pipe_volume / travel_time});
  moving_heat.cp = cp;
  const auto heated_route = fluid::heat(
      fluid::network(one_pipe()), transport_schedule, moving_heat,
      transport_options);
  const double expected_outlet = 300.0 + 40.0 * std::exp(-1.0);
  const double actual_outlet = heated_route.node.select(0, 1)
                                   .select(-1, 1)
                                   .item<double>();
  expect(std::abs(actual_outlet - expected_outlet) < 3e-8,
         "moving thermal plug did not match its residence-time solution: got " +
             std::to_string(actual_outlet) + ", expected " +
             std::to_string(expected_outlet));

  // Thermal junctions use the same complete, flow-weighted mixing system.
  fluid::Heat mixed_heat;
  mixed_heat.initial = pluss::series::make({300.0, 340.0, 300.0});
  mixed_heat.source = mixed_heat.initial;
  const auto thermally_mixed = fluid::heat(
      fluid::network(mixer), transport_schedule, mixed_heat,
      transport_options);
  expect(std::abs(thermally_mixed.node.select(0, 1)
                      .select(-1, 2)
                      .item<double>() -
                  320.0) < 2e-8,
         "equal thermal tributaries did not mix to their mean temperature");

  // Tank heat loss uses current hydraulic volume in the capacity term.  A
  // stagnant tank therefore reduces to the exact lumped-capacitance law.
  constexpr double tank_volume0 = 100.0;
  fluid::Heat tank_heat;
  tank_heat.initial = pluss::series::make({350.0, 350.0});
  tank_heat.tank_ambient = pluss::series::make({300.0});
  tank_heat.tank_ua = pluss::series::make(
      {density * cp * tank_volume0 * thermal_rate});
  tank_heat.cp = cp;
  const auto cooled_tank = fluid::heat(fluid::network(tank_pipe(0.0)),
                                       decay_schedule, tank_heat,
                                       decay_options);
  close(cooled_tank.tank.select(0, 1), {cooled_temperature}, 2e-8);

  // Scenario axes remain vectorized, and dtype follows the compiled network.
  fluid::Heat batched_heat;
  batched_heat.initial = pluss::series::make(
      std::vector<double>{320.0, 300.0, 340.0, 300.0}, {2, 2});
  batched_heat.source = batched_heat.initial;
  const auto heat_batch = fluid::heat(fluid::network(batched_pipe),
                                      transport_schedule, batched_heat,
                                      transport_options);
  expect(heat_batch.node.sizes() == std::vector<std::int64_t>({2, 2, 2}),
         "heat routing did not preserve the scenario axis");
  fluid::Heat float_heat;
  float_heat.initial =
      pluss::series::make({320.0, 300.0}).to(pluss::ScalarType::Float32);
  float_heat.source = float_heat.initial;
  const auto routed_float_heat = fluid::heat(
      fluid::network(float_pipe), transport_schedule, float_heat,
      transport_options, float_options);
  expect(routed_float_heat.node.scalar_type() == pluss::ScalarType::Float32 &&
             routed_float_heat.energy.scalar_type() ==
                 pluss::ScalarType::Float32,
         "heat routing did not retain Float32");

  // Both source temperature and UA remain on the reverse graph.
  auto tracked_temperature = pluss::series::make({340.0, 300.0});
  tracked_temperature.tracks_slope_(true);
  fluid::Heat tracked_heat;
  tracked_heat.initial = pluss::series::make({300.0, 300.0});
  tracked_heat.source = tracked_temperature;
  const auto tracked_thermal_route = fluid::heat(
      fluid::network(one_pipe()), transport_schedule, tracked_heat,
      transport_options);
  tracked_thermal_route.node.select(0, 1).select(-1, 1).reverse();
  close(tracked_temperature.sensitivity(), {1.0, 0.0}, 2e-7);

  auto tracked_ua = pluss::series::make(
      {density * cp * pipe_volume * thermal_rate});
  tracked_ua.tracks_slope_(true);
  fluid::Heat differentiable_heat = stagnant_heat;
  differentiable_heat.ua = tracked_ua;
  const auto differentiated = fluid::heat(
      fluid::network(one_pipe(0.0)), decay_schedule, differentiable_heat,
      decay_options);
  differentiated.link.select(0, 1).select(-1, 0).reverse();
  const double expected_ua_slope =
      -50.0 * 10.0 * std::exp(-1.0) / (density * cp * pipe_volume);
  close(tracked_ua.sensitivity(), {expected_ua_slope}, 2e-14);

  expect_error(
      [&] {
        fluid::RouteOptions bad = transport_options;
        bad.cells = 0;
        (void)fluid::transport(fluid::network(one_pipe()), transport_schedule,
                               constituent, bad);
      },
      "zero quality cells were accepted");
  expect_error(
      [&] {
        fluid::Quality bad;
        (void)fluid::transport(fluid::network(one_pipe()), transport_schedule,
                               bad, transport_options);
      },
      "a constituent run without initial quality was accepted");
  expect_error(
      [&] {
        fluid::Quality bad;
        bad.mode = fluid::QualityMode::Age;
        bad.pipe_rate = pluss::series::make({-0.1});
        (void)fluid::transport(fluid::network(one_pipe()), transport_schedule,
                               bad, transport_options);
      },
      "an external reaction field was accepted for water age");
  expect_error(
      [&] {
        fluid::Heat bad = stagnant_heat;
        bad.cp = 0.0;
        (void)fluid::heat(fluid::network(one_pipe(0.0)), decay_schedule, bad,
                          decay_options);
      },
      "zero heat capacity was accepted");
  expect_error(
      [&] {
        fluid::Heat bad = stagnant_heat;
        bad.ua = pluss::series::make({-1.0});
        (void)fluid::heat(fluid::network(one_pipe(0.0)), decay_schedule, bad,
                          decay_options);
      },
      "negative pipe UA was accepted");
  expect_error(
      [&] {
        fluid::Heat bad = stagnant_heat;
        bad.ambient = pluss::series::undefined();
        (void)fluid::heat(fluid::network(one_pipe(0.0)), decay_schedule, bad,
                          decay_options);
      },
      "positive pipe UA without an ambient temperature was accepted");
  expect_error(
      [&] {
        fluid::Heat bad = stagnant_heat;
        bad.initial = pluss::series::make({-1.0, 300.0});
        (void)fluid::heat(fluid::network(one_pipe(0.0)), decay_schedule, bad,
                          decay_options);
      },
      "negative absolute temperature was accepted");

  std::cout << "eng fluid tests passed\n";
  return 0;
}
