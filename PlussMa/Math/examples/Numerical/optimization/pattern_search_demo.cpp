#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

int main() {
  using namespace pluss::subsp::math::optimization;

  const pluss::Config cfg = pluss::Config{}
                                .with_dtype(pluss::ScalarType::Float64)
                                .with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::make_series(std::vector<double>{-1.2, 1.0}, {2}, cfg);

  PSConfig ps;
  ps.max_iterations = 800;
  ps.max_evaluations = 6000;
  ps.mesh_tolerance = 1e-8;
  ps.step_tolerance = 1e-8;
  ps.value_tolerance = 1e-8;
  ps.initial_mesh_size = 0.25;
  ps.mesh_expansion = 2.0;
  ps.mesh_contraction = 0.5;
  ps.poll_pattern = PSPollPattern::PositiveBasis2N;
  ps.polling_order = PSPollingOrder::SuccessFirst;
  ps.complete_poll = false;
  ps.direction_family = PSDirectionFamily::GSSAugmented;
  ps.basis_family = PSBasisFamily::RandomOrtho;
  ps.basis_refresh_interval = 2;
  ps.mesh_scaling = PSMeshScaling::NupsDynamic;
  ps.nups_expansion = 1.8;
  ps.nups_contraction = 0.7;
  ps.nups_decay = 0.97;
  ps.search_strategy = PSSearchStrategy::ExpandAndRandomDirections;
  ps.search_trials = 16;
  ps.search_complete = false;
  ps.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
  ps.al_max_outer_iterations = 5;
  ps.al_penalty_growth = 4.0;
  ps.al_violation_reduction_ratio = 0.6;
  ps.lower_bounds = pluss::series::full({2}, -2.0, cfg);
  ps.upper_bounds = pluss::series::full({2}, 2.0, cfg);
  ps.linear_ineq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}, cfg);
  ps.linear_ineq_rhs = pluss::make_series(std::vector<double>{1.5}, {1}, cfg);
  ps.nonlinear_constraints = [](const pluss::Series& x) {
    PSConstraintValues c;
    c.inequality = x.square().sum().sub(1.5).reshape({1}); // x0^2 + x1^2 <= 1.5
    return c;
  };

  const auto out = pattern_search_minimizer(
      [](const pluss::Series& x) {
        const pluss::Series x1 = x.select(0, 0);
        const pluss::Series x2 = x.select(0, 1);
        return x2.sub(x1.square()).square().mul(100.0).add(x1.sub(1.0).square());
      },
      x0,
      ps);

  const pluss::Series xmin = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64).reshape({2});

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "pattern_search_demo\n";
  std::cout << "status=" << to_string(out.status)
            << " f=" << out.minimum
            << " iter=" << out.diagnostics.iterations
            << " evals=" << out.diagnostics.evaluations
            << " strials=" << out.diagnostics.search_trials
            << " ssucc=" << out.diagnostics.search_successes
            << " mesh=" << out.diagnostics.mesh_size
            << " maxcv=" << out.diagnostics.max_constraint_violation
            << " step=" << out.diagnostics.last_step << "\n";
  std::cout << "x*=[" << xmin.select(0, 0).item<double>() << ", "
            << xmin.select(0, 1).item<double>() << "]\n";

  return 0;
}
