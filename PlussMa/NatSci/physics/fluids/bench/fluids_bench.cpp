#include "pluss/ma/natsci/physics.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;

using pluss::Platform;
using pluss::ScalarType;
using pluss::Series;

namespace {

using Clock = std::chrono::steady_clock;

struct Config final {
  std::int64_t size{64};
  std::int64_t rounds{5};
  std::int64_t warmup{2};
  std::string platform{"cpu"};
  std::string domain{"all"};
  bool csv{false};
};

struct Result final {
  std::string domain;
  std::string operation;
  std::string platform;
  std::int64_t size{};
  std::int64_t stages{};
  double median_ms{};
  double minimum_ms{};
  double cells_per_second{};
  double stage_cells_per_second{};
  double checksum{};
};

common::ScalarQuantity<common::Length> metres(double value) {
  return common::ScalarQuantity<common::Length>::from_si(value);
}

common::ScalarQuantity<common::Time> seconds(double value) {
  return common::ScalarQuantity<common::Time>::from_si(value);
}

common::ScalarQuantity<fluids::Density> density() {
  return common::ScalarQuantity<fluids::Density>::from_si(1.0);
}

common::ScalarQuantity<fluids::KinematicViscosity> viscosity() {
  return common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.01);
}

Series cpu_noise(std::initializer_list<std::int64_t> shape,
                 std::uint64_t seed) {
  return pluss::series::randn(shape,
                              pluss::Config{}
                                  .with_dtype(ScalarType::Float32)
                                  .with_platform(pluss::pCPU),
                              seed);
}

Series stack(const Series &x, const Series &y) {
  return x.unsqueeze(0).unite({y.unsqueeze(0)}, 0);
}

fluids::ProjectionOptions projection_options(std::int64_t size) {
  fluids::ProjectionOptions options;
  options.poisson.solve.max_iterations = std::max<std::int64_t>(256, 4 * size);
  options.poisson.solve.relative_tolerance = 2e-4;
  options.poisson.solve.absolute_tolerance = 1e-5;
  options.poisson.solve.check_interval = 8;
  // Two discrete derivatives amplify Float32 cancellation noise by h^-2.
  // Scale only the null-space compatibility check; solve tolerances stay fixed.
  options.poisson.mean_tolerance =
      std::max(2e-5, 5e-7 * static_cast<double>(size * size));
  options.poisson.require_convergence = true;
  return options;
}

fluids::StepOptions step_options(fluids::StepRule rule, std::int64_t size) {
  fluids::StepOptions options;
  options.rule = rule;
  options.guard.enforce = false;
  options.projection = projection_options(size);
  return options;
}

std::int64_t stages(fluids::StepRule rule) {
  if (rule == fluids::StepRule::Euler)
    return 1;
  if (rule == fluids::StepRule::Rk2)
    return 2;
  return 3;
}

std::string_view rule_name(fluids::StepRule rule) {
  if (rule == fluids::StepRule::Euler)
    return "euler";
  if (rule == fluids::StepRule::Rk2)
    return "rk2";
  return "rk3";
}

double synchronize(const Series &value) {
  return static_cast<double>(value.sum().item<float>());
}

template <class Fn>
Result measure(std::string domain, std::string operation, std::string platform,
               std::int64_t size, std::int64_t stages, std::int64_t warmup,
               std::int64_t rounds, Fn &&fn) {
  double checksum = 0.0;
  for (std::int64_t round = 0; round < warmup; ++round)
    checksum += synchronize(fn());

  std::vector<double> times;
  times.reserve(static_cast<std::size_t>(rounds));
  for (std::int64_t round = 0; round < rounds; ++round) {
    const auto start = Clock::now();
    checksum += synchronize(fn());
    const auto stop = Clock::now();
    times.push_back(
        std::chrono::duration<double, std::milli>(stop - start).count());
  }
  std::sort(times.begin(), times.end());
  const double median = times[times.size() / 2];
  const double cells = static_cast<double>(size) * static_cast<double>(size);
  const double seconds_per_round = median / 1'000.0;
  return {std::move(domain),
          std::move(operation),
          std::move(platform),
          size,
          stages,
          median,
          times.front(),
          cells / seconds_per_round,
          cells * static_cast<double>(stages) / seconds_per_round,
          checksum};
}

struct PeriodicCase final {
  spatial::PeriodicGrid grid;
  fluids::IncompressibleState state;
  fluids::IncompressibleState divergent;
};

PeriodicCase periodic_case(std::int64_t n, Platform platform) {
  const double h = 1.0 / static_cast<double>(n);
  auto grid = spatial::make_periodic_grid({n, n}, {metres(h), metres(h)});
  const Series stream = cpu_noise({n, n}, 41).to(platform);
  const Series potential = cpu_noise({n, n}, 73).to(platform);
  const Series stream_gradient = spatial::gradient(stream, grid);
  const Series velocity =
      stack(stream_gradient.select(0, 1), stream_gradient.select(0, 0).neg())
          .mul(0.01F);
  const Series pressure = pluss::series::zeros_like(stream);
  auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
  const Series divergent_velocity =
      velocity.add(spatial::gradient(potential, grid).mul(0.001F));
  auto divergent = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(divergent_velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
  return {std::move(grid), std::move(state), std::move(divergent)};
}

struct BoundedCase final {
  spatial::BoxGrid grid;
  fluids::FlowBounds bounds;
  fluids::IncompressibleState state;
  fluids::IncompressibleState divergent;
};

BoundedCase bounded_case(std::int64_t n, Platform platform) {
  const double h = 1.0 / static_cast<double>(n);
  auto grid = spatial::make_box_grid({n, n}, {metres(h), metres(h)});
  const auto lid = common::SeriesQuantity<common::Speed>::from_si(
      pluss::series::make({1.0F, 0.0F}).to(platform));
  auto bounds = fluids::flow_bounds(
      grid, {fluids::no_slip({0, spatial::Side::Low}),
             fluids::no_slip({0, spatial::Side::High}),
             fluids::no_slip({1, spatial::Side::Low}),
             fluids::no_slip({1, spatial::Side::High}, lid)});
  const Series velocity =
      pluss::series::zeros({2, n, n}, ScalarType::Float32).to(platform);
  const Series pressure =
      pluss::series::zeros({n, n}, ScalarType::Float32).to(platform);
  auto state = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(velocity),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
  const Series noisy = cpu_noise({2, n, n}, 97).to(platform).mul(0.001F);
  auto raw = fluids::make_state(
      common::SeriesQuantity<common::Speed>::from_si(noisy),
      common::SeriesQuantity<common::Pressure>::from_si(pressure), grid);
  return {std::move(grid), std::move(bounds), std::move(state), std::move(raw)};
}

void append_periodic(std::vector<Result> &results, const Config &config,
                     Platform platform, std::string_view platform_name) {
  const auto options = projection_options(config.size);
  const auto setup = periodic_case(config.size, platform);
  const auto width = seconds(0.1 / static_cast<double>(config.size));
  results.push_back(measure("periodic", "rate", std::string(platform_name),
                            config.size, 1, config.warmup, config.rounds, [&] {
                              return fluids::mom_rate(setup.state, setup.grid,
                                                      density(), viscosity())
                                  .si_value();
                            }));
  results.push_back(measure("periodic", "project", std::string(platform_name),
                            config.size, 1, config.warmup, config.rounds, [&] {
                              return fluids::project(setup.divergent,
                                                     setup.grid, density(),
                                                     seconds(1.0), options)
                                  .state.velocity()
                                  .si_value();
                            }));
  for (const auto rule : {fluids::StepRule::Euler, fluids::StepRule::Rk2,
                          fluids::StepRule::Rk3}) {
    const auto step = step_options(rule, config.size);
    results.push_back(measure(
        "periodic", std::string(rule_name(rule)), std::string(platform_name),
        config.size, stages(rule), config.warmup, config.rounds, [&] {
          return fluids::advance(setup.state, setup.grid, density(),
                                 viscosity(), seconds(0.0), width, step)
              .state.velocity()
              .si_value();
        }));
  }
}

void append_bounded(std::vector<Result> &results, const Config &config,
                    Platform platform, std::string_view platform_name) {
  const auto options = projection_options(config.size);
  const auto setup = bounded_case(config.size, platform);
  const auto width = seconds(0.1 / static_cast<double>(config.size));
  results.push_back(measure("bounded", "rate", std::string(platform_name),
                            config.size, 1, config.warmup, config.rounds, [&] {
                              return fluids::mom_rate(setup.state, setup.grid,
                                                      setup.bounds, density(),
                                                      viscosity())
                                  .si_value();
                            }));
  results.push_back(measure("bounded", "project", std::string(platform_name),
                            config.size, 1, config.warmup, config.rounds, [&] {
                              return fluids::project(setup.divergent,
                                                     setup.grid, setup.bounds,
                                                     density(), seconds(1.0),
                                                     options)
                                  .state.velocity()
                                  .si_value();
                            }));
  for (const auto rule : {fluids::StepRule::Euler, fluids::StepRule::Rk2,
                          fluids::StepRule::Rk3}) {
    const auto step = step_options(rule, config.size);
    results.push_back(measure(
        "bounded", std::string(rule_name(rule)), std::string(platform_name),
        config.size, stages(rule), config.warmup, config.rounds, [&] {
          return fluids::advance(setup.state, setup.grid, setup.bounds,
                                 density(), viscosity(), seconds(0.0), width,
                                 step)
              .state.velocity()
              .si_value();
        }));
  }
}

Config parse(int argc, char **argv) {
  Config config;
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    const auto value = [&](std::string_view prefix) {
      return argument.substr(prefix.size());
    };
    if (argument.starts_with("--size="))
      config.size = std::stoll(value("--size="));
    else if (argument.starts_with("--rounds="))
      config.rounds = std::stoll(value("--rounds="));
    else if (argument.starts_with("--warmup="))
      config.warmup = std::stoll(value("--warmup="));
    else if (argument.starts_with("--platform="))
      config.platform = value("--platform=");
    else if (argument.starts_with("--domain="))
      config.domain = value("--domain=");
    else if (argument == "--csv")
      config.csv = true;
    else
      throw std::invalid_argument("unknown option: " + argument);
  }
  if (config.size < 4 || config.rounds <= 0 || config.warmup < 0)
    throw std::invalid_argument(
        "size must be at least 4, rounds positive, and warmup non-negative");
  if (config.platform != "cpu" && config.platform != "mps" &&
      config.platform != "all")
    throw std::invalid_argument("platform must be cpu, mps, or all");
  if (config.domain != "periodic" && config.domain != "bounded" &&
      config.domain != "all")
    throw std::invalid_argument("domain must be periodic, bounded, or all");
  return config;
}

void print(const std::vector<Result> &results, bool csv) {
  if (csv) {
    std::cout << "domain,operation,platform,size,stages,median_ms,minimum_ms,"
                 "cells_per_second,stage_cells_per_second,checksum\n";
    for (const auto &result : results)
      std::cout << result.domain << ',' << result.operation << ','
                << result.platform << ',' << result.size << ',' << result.stages
                << ',' << result.median_ms << ',' << result.minimum_ms << ','
                << result.cells_per_second << ','
                << result.stage_cells_per_second << ',' << result.checksum
                << '\n';
    return;
  }

  std::cout << std::left << std::setw(10) << "domain" << std::setw(10)
            << "operation" << std::setw(10) << "platform" << std::right
            << std::setw(8) << "size" << std::setw(8) << "stages"
            << std::setw(14) << "median ms" << std::setw(18) << "cells/s"
            << std::setw(20) << "stage cells/s" << '\n';
  for (const auto &result : results)
    std::cout << std::left << std::setw(10) << result.domain << std::setw(10)
              << result.operation << std::setw(10) << result.platform
              << std::right << std::setw(8) << result.size << std::setw(8)
              << result.stages << std::setw(14) << std::fixed
              << std::setprecision(3) << result.median_ms << std::setw(18)
              << std::scientific << std::setprecision(3)
              << result.cells_per_second << std::setw(20)
              << result.stage_cells_per_second << '\n';
}

} // namespace

int main(int argc, char **argv) {
  try {
    const Config config = parse(argc, argv);
    pluss::NoGradGuard no_grad;
    std::vector<Result> results;
    const auto run = [&](Platform platform, std::string_view name) {
      if (config.domain == "periodic" || config.domain == "all")
        append_periodic(results, config, platform, name);
      if (config.domain == "bounded" || config.domain == "all")
        append_bounded(results, config, platform, name);
    };
    if (config.platform == "cpu" || config.platform == "all")
      run(pluss::pCPU, "cpu");
    if (config.platform == "mps" || config.platform == "all") {
      if (!pluss::codem::mps_backend_available())
        throw std::runtime_error("MPS was requested but is unavailable");
      run(pluss::pMPS, "mps");
    }
    print(results, config.csv);
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
