#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <algorithm>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec(std::int64_t n, double start, double step) {
  std::vector<double> values(static_cast<std::size_t>(n), 0.0);
  for (std::int64_t i = 0; i < n; ++i) values[static_cast<std::size_t>(i)] = start + step * static_cast<double>(i);
  return make_series(std::move(values), {n}).to(ScalarType::Float64).to(pluss::pCPU);
}

} // namespace

TEST_CASE("interior-point can use conjugate-gradient subproblem solves", "[optim][optima][cg]") {
  const std::int64_t n = 24;
  const Series target = make_vec(n, 0.2, 0.05);
  const Series initial_x = make_vec(n, 1.5, 0.01);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) -> Trace {
    const Trace err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", pluss::zeros({n}, ScalarType::Float64).to(pluss::pCPU)},
  });

  const ParameterPack<Series> initial({
      {"x", initial_x},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::ConjugateGradient;
  options.internal_protocol.max_krylov_iterations = 128;
  options.internal_protocol.krylov_tolerance = 1e-6;
  options.internal_protocol.krylov_absolute_tolerance = 1e-10;
  std::int64_t max_krylov_seen = 0;
  options.internal_protocol.progress_hook = [&max_krylov_seen](const auto& snapshot) {
    max_krylov_seen = std::max(max_krylov_seen, snapshot.krylov_iterations);
    return false;
  };

  const auto result = optima(problem, initial, options);

  REQUIRE(result.converged);
  REQUIRE(max_krylov_seen > 0);
  REQUIRE(result.evaluation.value.item<double>() < 1e-6);
  REQUIRE(result.params.at("x").sub(target).abs().max().item<double>() == Approx(0.0).margin(1e-3));
}
