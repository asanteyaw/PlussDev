#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

ConstrainedProblem make_scaled_quadratic_problem() {
  const Series target = make_vec2(1000.0, 1.0e-3);
  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) -> Trace {
    const Trace err = p.at("x") - trace(target, false);
    return sum(square(err * trace(make_vec2(1.0, 1000.0), false)));
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

ParameterPack<Series> make_scaled_quadratic_initial() {
  return ParameterPack<Series>({
      {"x", make_vec2(10.0, 1.0)},
  });
}

} // namespace

TEST_CASE("optima scaling path converges on an ill-scaled bounded problem",
          "[optim][optima][scaling]") {
  ConstrainedProblem problem = make_scaled_quadratic_problem();
  const auto initial = make_scaled_quadratic_initial();

  OptimaOptions sqp_options;
  sqp_options.algorithm = OptimaAlgorithm::SequentialQP;
  sqp_options.sequential_qp.max_iterations = 40;
  sqp_options.sequential_qp.problem_scaling = OptimaProblemScaling::InitialPoint;

  const auto sqp = optima(problem, initial, sqp_options);
  REQUIRE(sqp.converged);
  REQUIRE(sqp.evaluation.value.item<double>() < 1e-6);
  REQUIRE(sqp.params.at("x").select(0, 0).item<double>() == Approx(1000.0).margin(1e-3));
  REQUIRE(sqp.params.at("x").select(0, 1).item<double>() == Approx(1.0e-3).margin(1e-6));

  OptimaOptions ip_options;
  ip_options.algorithm = OptimaAlgorithm::InternalProtocol;
  ip_options.internal_protocol.max_iterations = 50;
  ip_options.internal_protocol.problem_scaling = OptimaProblemScaling::InitialPoint;

  const auto ip = optima(problem, initial, ip_options);
  REQUIRE(ip.converged);
  REQUIRE(ip.evaluation.value.item<double>() < 1e-6);
  REQUIRE(ip.params.at("x").select(0, 0).item<double>() == Approx(1000.0).margin(1e-3));
  REQUIRE(ip.params.at("x").select(0, 1).item<double>() == Approx(1.0e-3).margin(1e-6));
}
