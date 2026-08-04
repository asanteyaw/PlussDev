#include <pluss/ma.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {

double as_f64_scalar(const pluss::Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

pluss::Series ackley(const pluss::Series& x) {
  const pluss::Series sq_mean = x.square().mean();
  const pluss::Series cos_mean = x.mul(2.0 * 3.14159265358979323846).cos().mean();
  const pluss::Series twenty = pluss::make_series(20.0);
  const pluss::Series e = pluss::make_series(2.71828182845904523536);
  return twenty.add(e).sub(sq_mean.sqrt().mul(-0.2).exp().mul(20.0)).sub(cos_mean.exp());
}

pluss::Series sphere(const pluss::Series& x) {
  return x.square().sum();
}

pluss::Series sum_le_one_constraint(const pluss::Series& x) {
  return x.sum().sub(1.0).reshape({1});
}

void print_result(
    const std::string& label,
    const pluss::subsp::math::optimization::DfoResult& out) {
  const pluss::Series norm = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64).square().sum().sqrt();
  std::cout << label
            << " solver=" << pluss::subsp::math::optimization::to_string(out.selected_solver)
            << " status=" << pluss::subsp::math::optimization::to_string(out.status)
            << " f=" << out.minimum
            << " ||x||=" << as_f64_scalar(norm)
            << " evals=" << out.diagnostics.evaluations
            << " reason=\"" << out.selection_reason << "\"\n";
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x0 = pluss::series::full({12}, 2.5, cfg);

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "Unified DFO facade demo\n";

  DfoProblem unconstrained;
  unconstrained.initial = x0;
  unconstrained.objective = ackley;

  DfoOptions auto_opts;
  auto_opts.max_iterations = 700;
  auto_opts.max_evaluations = 20000;
  auto_opts.check_every = 10;
  auto_opts.rng_seed = 20260226ULL;

  const DfoResult auto_out = DFO::solve(unconstrained, auto_opts);
  print_result("auto", auto_out);

  DfoProblem unconstrained_small;
  unconstrained_small.initial = pluss::series::full({2}, 2.5, cfg);
  unconstrained_small.objective = sphere;

  DfoOptions forced_nm = auto_opts;
  forced_nm.solver = DfoSolverKind::NelderMead;
  forced_nm.max_iterations = 1200;
  forced_nm.max_evaluations = 12000;
  const DfoResult nm_out = DFO::solve(unconstrained_small, forced_nm);
  print_result("forced_nelder_mead_sphere", nm_out);

  DfoProblem constrained;
  constrained.initial = pluss::series::full({6}, 1.5, cfg);
  constrained.objective = sphere;
  constrained.constraints = sum_le_one_constraint;
  constrained.lower_bounds = pluss::series::full({6}, -2.0, cfg);
  constrained.upper_bounds = pluss::series::full({6}, 2.0, cfg);

  DfoOptions constrained_opts;
  constrained_opts.max_iterations = 500;
  constrained_opts.max_evaluations = 4000;
  constrained_opts.max_total_evaluations = 6000;
  constrained_opts.rng_seed = 20260227ULL;
  constrained_opts.mads.constraint_handling = ConstraintHandling::ProgressiveBarrier;
  const DfoResult constrained_out = DFO::solve(constrained, constrained_opts);
  print_result("auto_constrained", constrained_out);

  const DfoResult builder_out = DFO::build(pluss::series::full({4}, 2.0, cfg))
                                    .with_objective(sphere)
                                    .with_bounds(pluss::series::full({4}, -1.0, cfg), pluss::series::full({4}, 1.0, cfg))
                                    .with_max_iterations(250)
                                    .with_max_evaluations(10000)
                                    .with_seed(20260228ULL)
                                    .solve();
  print_result("builder_boxed_auto", builder_out);

  return 0;
}
