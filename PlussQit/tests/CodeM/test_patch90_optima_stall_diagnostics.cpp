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
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::InternalProtocolState;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaProgressStage;
using pluss::subsp::math::optim::OptimaStallCategory;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::PenaltyMethodState;
using pluss::subsp::math::optim::SequentialQPState;
using pluss::subsp::math::optim::detail::make_progress_snapshot_;
using pluss::subsp::math::optim::optima_stall_category_name;
using pluss::subsp::math::optim::to_optima_state;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

Series make_scalar(double x) {
  return make_series(x).to(ScalarType::Float64);
}

Series make_empty() {
  return pluss::empty({0}, ScalarType::Float64).to(pluss::pCPU);
}

ParameterPack<Series> make_params() {
  return ParameterPack<Series>({
      {"x", make_vec2(0.5, 0.5)},
  });
}

} // namespace

TEST_CASE("optima exposes boundary-limited stall diagnostics for SQP output and progress snapshots",
          "[optim][optima][stall][sqp][boundary]") {
  SequentialQPState state;
  state.params = make_params();
  state.iteration = 8;
  state.function_evaluations = 21;
  state.last_step_size = 0.0;
  state.current_penalty = 10.0;
  state.kkt_residual = 2.5e-3;
  state.krylov_iterations = 6;
  state.converged = true;
  state.message = "no progress in the bound-feasible region";
  state.evaluation.status = EvalStatus::Ok;
  state.evaluation.value = make_scalar(1.25);
  state.evaluation.gradient = make_params().zeros_like();
  state.evaluation.nonlinear_equality_residual = make_empty();
  state.evaluation.nonlinear_inequality_residual = make_empty();
  state.evaluation.equality_residual = make_empty();
  state.evaluation.inequality_residual = make_empty();
  state.evaluation.max_constraint_violation = 2e-3;
  state.evaluation.first_order_opt = 2.5e-3;

  const auto result = to_optima_state(ConstrainedProblem{}, state);
  const auto snapshot = make_progress_snapshot_(OptimaAlgorithm::SequentialQP, OptimaProgressStage::Done, state);

  REQUIRE(result.output.stall_category == OptimaStallCategory::BoundaryLimited);
  REQUIRE(std::string(optima_stall_category_name(result.output.stall_category)) == "boundary-limited");
  REQUIRE(result.output.stall_measure == Approx(0.0).margin(1e-16));
  REQUIRE(result.output.detail_message.find("diagnostic=boundary-limited") != std::string::npos);
  REQUIRE(snapshot.stall_category == OptimaStallCategory::BoundaryLimited);
  REQUIRE(snapshot.krylov_iterations == 6);
}

TEST_CASE("optima exposes complementarity stagnation diagnostics for interior-point iteration limits",
          "[optim][optima][stall][ip][complementarity]") {
  InternalProtocolState state;
  state.params = make_params();
  state.iteration = 40;
  state.function_evaluations = 97;
  state.last_step_size = 1e-3;
  state.barrier_parameter = 1e-6;
  state.barrier_gradient_norm = 1e-6;
  state.complementarity_residual = 2e-2;
  state.krylov_iterations = 11;
  state.converged = true;
  state.message = "maximum iterations reached";
  state.evaluation.status = EvalStatus::Ok;
  state.evaluation.value = make_scalar(0.75);
  state.evaluation.gradient = make_params().zeros_like();
  state.evaluation.nonlinear_equality_residual = make_empty();
  state.evaluation.nonlinear_inequality_residual = make_empty();
  state.evaluation.equality_residual = make_empty();
  state.evaluation.inequality_residual = make_empty();
  state.evaluation.max_constraint_violation = 5e-7;
  state.evaluation.first_order_opt = 1e-6;

  const auto result = to_optima_state(ConstrainedProblem{}, state);
  const auto snapshot = make_progress_snapshot_(OptimaAlgorithm::InternalProtocol, OptimaProgressStage::Done, state);

  REQUIRE(result.output.stall_category == OptimaStallCategory::ComplementarityStagnation);
  REQUIRE(result.output.stall_measure == Approx(2e-2).margin(1e-12));
  REQUIRE(result.output.stall_reference == Approx(1e-6).margin(1e-12));
  REQUIRE(result.output.detail_message.find("diagnostic=complementarity-stagnation") != std::string::npos);
  REQUIRE(snapshot.stall_category == OptimaStallCategory::ComplementarityStagnation);
  REQUIRE(snapshot.complementarity == Approx(2e-2).margin(1e-12));
  REQUIRE(snapshot.krylov_iterations == 11);
}

TEST_CASE("optima classifies restoration failures separately from generic line-search failure",
          "[optim][optima][stall][recovery]") {
  SequentialQPState state;
  state.params = make_params();
  state.iteration = 5;
  state.function_evaluations = 18;
  state.last_step_size = 0.0;
  state.current_penalty = 20.0;
  state.kkt_residual = 0.1;
  state.converged = true;
  state.message = "restoration failed to reduce constraint violation";
  state.evaluation.status = EvalStatus::Ok;
  state.evaluation.value = make_scalar(3.0);
  state.evaluation.gradient = make_params().zeros_like();
  state.evaluation.nonlinear_equality_residual = make_empty();
  state.evaluation.nonlinear_inequality_residual = make_empty();
  state.evaluation.equality_residual = make_empty();
  state.evaluation.inequality_residual = make_empty();
  state.evaluation.max_constraint_violation = 0.8;
  state.evaluation.first_order_opt = 0.1;

  const auto result = to_optima_state(ConstrainedProblem{}, state);

  REQUIRE(result.output.stall_category == OptimaStallCategory::RecoveryFailure);
  REQUIRE(result.output.detail_message.find("diagnostic=recovery-failure") != std::string::npos);
}

TEST_CASE("optima classifies non-finite evaluation regions explicitly",
          "[optim][optima][stall][nonfinite]") {
  PenaltyMethodState state;
  state.params = make_params();
  state.iteration = 0;
  state.function_evaluations = 1;
  state.converged = true;
  state.message = "objective evaluation returned non-finite values";
  state.evaluation.status = EvalStatus::NonFiniteObjective;

  const auto result = to_optima_state(ConstrainedProblem{}, state);

  REQUIRE(result.output.stall_category == OptimaStallCategory::NonFiniteCallbackRegion);
  REQUIRE(result.output.detail_message.find("diagnostic=nonfinite-callback-region") != std::string::npos);
}
