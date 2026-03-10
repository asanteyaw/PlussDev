#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <stdexcept>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProgressSnapshot;
using pluss::subsp::math::optim::OptimaProgressStage;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

ConstrainedProblem make_progress_problem() {
  ConstrainedProblem problem;
  const Series target = make_vec2(1.0, 2.0);
  problem.objective.autodiff = [target](const ParameterPack<pluss::subsp::math::optim::Trace>& p) {
    const auto err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.nonlinear.inequality = [](const ParameterPack<pluss::subsp::math::optim::Trace>& p) {
    return sum(p.at("x")) - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

} // namespace

TEST_CASE("optima progress hook can stop sequential qp iterations", "[optim][optima][hook][stop]") {
  const auto problem = make_progress_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  std::vector<OptimaProgressStage> stages;
  std::int64_t iteration_callbacks = 0;

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.progress_hook = [&](const OptimaProgressSnapshot& snapshot) {
    stages.push_back(snapshot.stage);
    if (snapshot.stage == OptimaProgressStage::Iteration) {
      ++iteration_callbacks;
      return true;
    }
    return false;
  };

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::Stopped);
  REQUIRE(result.output.detail_message.find("progress hook") != std::string::npos);
  REQUIRE(iteration_callbacks == 1);
  REQUIRE(stages.size() >= 3);
  REQUIRE(stages.front() == OptimaProgressStage::Start);
  REQUIRE(stages[1] == OptimaProgressStage::Iteration);
  REQUIRE(stages.back() == OptimaProgressStage::Done);
}

TEST_CASE("optima progress hook failures are contained and reported", "[optim][optima][hook][throw]") {
  const auto problem = make_progress_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.6, 0.6)},
  });

  bool done_seen = false;

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 40;
  options.progress_hook = [&](const OptimaProgressSnapshot& snapshot) {
    if (snapshot.stage == OptimaProgressStage::Done) {
      done_seen = true;
      return false;
    }
    if (snapshot.stage == OptimaProgressStage::Start) {
      throw std::runtime_error("hook boom");
    }
    return false;
  };

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::Stopped);
  REQUIRE(result.output.detail_message.find("progress hook failed") != std::string::npos);
  REQUIRE(done_seen);
}
