// Beyond the exit flag, every result carries a stall category that says what kind of difficulty ended
// the run. This program triggers each of the ten categories and prints its name, the exit flag, and the
// measure the solver recorded. A healthy run reports "none".
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <string>
#include <utility>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using namespace pluss::tils;

int main() {
  std::vector<std::pair<std::string, optim::OptimaState>> runs;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return p.at("x").square().sum() + 1.0; });

  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;

  runs.emplace_back("a normal solve", optim::optima(objective, vec({0.0, 0.0})));

  optim::OptimaOptions hook;
  hook.progress_hook = [](const optim::OptimaProgressSnapshot&) { return true; };
  runs.emplace_back("the progress hook asks to stop", optim::optima(objective, vec({0.0, 0.0}), hook));

  runs.emplace_back("the objective returns a vector",
         optim::optima([](const pluss::Series& x) { return x.square(); }, vec({1.0, 1.0})));

  runs.emplace_back("the objective is not finite at the start",
         optim::optima([](const pluss::Series& x) { return x.log().sum(); }, vec({-1.0, 1.0})));

  optim::OptimaOptions merit;
  merit.algorithm = optim::OptimaAlgorithm::MeritMethod;
  runs.emplace_back("merit method on an impossible inequality", optim::optima(objective, vec({0.0, 0.0}), merit, impossible_inequality));

  optim::OptimaOptions no_step = interior;
  no_step.internal_protocol.fraction_to_boundary = 0.0;
  runs.emplace_back("no step is allowed toward the boundary", optim::optima(objective, vec({0.0, 0.0}), no_step, disc));

  runs.emplace_back("sequential QP cannot restore feasibility", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  runs.emplace_back("interior method stalls on an infeasible equality",
         optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  optim::OptimaOptions phase = interior;
  phase.internal_protocol.enable_feasibility_phase = true;
  runs.emplace_back("feasibility phase cannot reduce the violation", optim::optima(objective, vec({1.0, 1.0}), phase, impossible_equality));

  optim::OptimaOptions short_run = interior;
  short_run.internal_protocol.max_iterations = 2;
  runs.emplace_back("interior method stops with barrier complementarity unresolved",
         optim::optima(objective, vec({0.1, 0.1}), short_run, disc));

  optim::OptimaOptions one_iteration;
  one_iteration.sequential_qp.max_iterations = 1;
  runs.emplace_back("the iteration budget is spent", optim::optima(objective, vec({0.0, 0.0}), one_iteration, disc));

  OutputEngine out;
  for (const auto& [scenario, r] : runs) {
    out.writeln(scenario);
    out.writeln("    stall =", optim::optima_stall_category_name(r.output.stall_category), " flag =", static_cast<int>(r.exit_flag));
  }
  return 0;
}
