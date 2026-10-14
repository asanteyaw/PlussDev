// Beyond the exit flag, every result carries a stall category that says what kind of difficulty ended
// the run. This program triggers each of the ten categories and prints its name, the exit flag, and the
// measure the solver recorded. A healthy run reports "none".
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

void report(const std::string& scenario, const optim::OptimaState& r) {
  std::cout << scenario << "\n    stall=" << optim::optima_stall_category_name(r.output.stall_category)
            << "  flag=" << static_cast<int>(r.exit_flag) << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;

  report("a normal solve", optim::optima(objective, vec({0.0, 0.0})));

  optim::OptimaOptions hook;
  hook.progress_hook = [](const optim::OptimaProgressSnapshot&) { return true; };
  report("the progress hook asks to stop", optim::optima(objective, vec({0.0, 0.0}), hook));

  report("the objective returns a vector",
         optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0})));

  report("the objective is not finite at the start",
         optim::optima([](const optim::Trace& x) { return optim::sum(optim::log(x)); }, vec({-1.0, 1.0})));

  optim::OptimaOptions merit;
  merit.algorithm = optim::OptimaAlgorithm::MeritMethod;
  report("merit method on an impossible inequality", optim::optima(objective, vec({0.0, 0.0}), merit, impossible_inequality));

  optim::OptimaOptions no_step = interior;
  no_step.internal_protocol.fraction_to_boundary = 0.0;
  report("no step is allowed toward the boundary", optim::optima(objective, vec({0.0, 0.0}), no_step, disc));

  report("sequential QP cannot restore feasibility", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  report("interior method stalls on an infeasible equality",
         optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  optim::OptimaOptions phase = interior;
  phase.internal_protocol.enable_feasibility_phase = true;
  report("feasibility phase cannot reduce the violation", optim::optima(objective, vec({1.0, 1.0}), phase, impossible_equality));

  optim::OptimaOptions short_run = interior;
  short_run.internal_protocol.max_iterations = 2;
  report("interior method stops with barrier complementarity unresolved",
         optim::optima(objective, vec({0.1, 0.1}), short_run, disc));

  optim::OptimaOptions one_iteration;
  one_iteration.sequential_qp.max_iterations = 1;
  report("the iteration budget is spent", optim::optima(objective, vec({0.0, 0.0}), one_iteration, disc));
  return 0;
}
