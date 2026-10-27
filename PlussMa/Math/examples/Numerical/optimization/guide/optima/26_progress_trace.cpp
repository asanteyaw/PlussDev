// reports::trace turns any stream into a progress hook that prints one line per iteration, so a run
// can be watched as it happens. reports::iter is the short form: iterations only, no start or
// final line, no message. ProgressTracePreset switches individual parts on and off.
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  std::cout << "-- reports::trace, default preset --\n";
  optim::OptimaOptions full;
  full.progress_hook = optim::reports::trace(std::cout);
  (void)optim::optima(objective, vec({0.0, 0.0}), full, disc);

  std::cout << "-- reports::iter, three decimals --\n";
  optim::OptimaOptions brief;
  brief.progress_hook = optim::reports::iter(std::cout, 3);
  (void)optim::optima(objective, vec({0.0, 0.0}), brief, disc);

  std::cout << "-- a custom preset: no start line, no final line, no message, no Krylov count --\n";
  optim::reports::ProgressTracePreset preset;
  preset.include_start = false;
  preset.include_done = false;
  preset.include_message = false;
  preset.include_krylov = false;
  preset.precision = 4;
  optim::OptimaOptions custom;
  custom.progress_hook = optim::reports::trace(std::cout, preset);
  (void)optim::optima(objective, vec({0.0, 0.0}), custom, disc);
  return 0;
}
