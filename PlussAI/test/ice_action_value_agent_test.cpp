#include <pluss/ai/ice/models/rl/action_value_agent.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::ice::models::ActionValueAgent;
using pluss::ai::ice::models::ActionValueAgentOptions;
using pluss::ai::ice::models::ActionValueBatch;
using pluss::ai::ice::models::ActionValueTrainOptions;

[[nodiscard]] Config f32() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

[[nodiscard]] ActionValueBatch make_chain_batch(std::int64_t repeats) {
  std::vector<float> obs{};
  std::vector<float> next_obs{};
  std::vector<std::int64_t> action{};
  std::vector<float> reward{};
  std::vector<float> done{};
  obs.reserve(static_cast<std::size_t>(repeats * 4 * 2));
  next_obs.reserve(static_cast<std::size_t>(repeats * 4 * 2));
  action.reserve(static_cast<std::size_t>(repeats * 4));
  reward.reserve(static_cast<std::size_t>(repeats * 4));
  done.reserve(static_cast<std::size_t>(repeats * 4));

  for (std::int64_t i = 0; i < repeats; ++i) {
    // state A, good action -> state B
    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 1.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(0.0F);

    // state A, bad action -> terminal
    obs.insert(obs.end(), {1.0F, 0.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(0.0F);
    done.push_back(1.0F);

    // state B, good action -> terminal reward
    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(0);
    reward.push_back(1.0F);
    done.push_back(1.0F);

    // state B, bad action -> terminal no reward
    obs.insert(obs.end(), {0.0F, 1.0F});
    next_obs.insert(next_obs.end(), {0.0F, 0.0F});
    action.push_back(1);
    reward.push_back(0.0F);
    done.push_back(1.0F);
  }

  const auto rows = static_cast<std::int64_t>(action.size());
  const auto cfg = f32();
  return ActionValueBatch{
      .observation = pluss::make_series(obs, {rows, 2}, cfg),
      .action = pluss::make_series(action, {rows}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU)),
      .reward = pluss::make_series(reward, {rows}, cfg),
      .next_observation = pluss::make_series(next_obs, {rows, 2}, cfg),
      .done = pluss::make_series(done, {rows}, cfg),
  };
}

bool run_ice_action_value_agent_checks(std::ostream* err = nullptr) {
  ActionValueAgentOptions model_options{};
  model_options.observation_size = 2;
  model_options.action_count = 2;
  model_options.hidden_size = 16;
  model_options.hidden_layers = 2;
  model_options.discount = 0.9;
  model_options.target_sync_every = 8;
  model_options.seed = 21;

  ActionValueAgent agent(model_options);
  if (agent.ready()) {
    if (err) *err << "ice ActionValueAgent should not report ready before learn()\n";
    return false;
  }

  ActionValueTrainOptions train_options{};
  train_options.batch_size = 32;
  train_options.learning_rate = 1e-2;
  train_options.execution.max_epochs = 120;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 12;
  train_options.loop_regularization.restore_best_weights = true;
  train_options.seed = 5;

  const auto batch = make_chain_batch(64);
  agent.learn(batch, train_options);
  if (!agent.ready()) {
    if (err) *err << "ice ActionValueAgent should report ready after learn()\n";
    return false;
  }

  const auto report = agent.assess(batch);
  if (!(report.loss >= 0.0 && report.loss < 0.1)) {
    if (err) *err << "ice ActionValueAgent should fit the tiny chain; got loss=" << report.loss << "\n";
    return false;
  }

  const auto cfg = f32();
  const Series state_a = pluss::make_series(std::vector<float>{1.0F, 0.0F}, {2}, cfg);
  const Series state_b = pluss::make_series(std::vector<float>{0.0F, 1.0F}, {2}, cfg);
  if (agent.act(state_a) != 1 || agent.act(state_b) != 0) {
    if (err) *err << "ice ActionValueAgent should learn the optimal action on both states\n";
    return false;
  }

  const Series stacked = pluss::make_series(std::vector<float>{1.0F, 0.0F, 0.0F, 1.0F}, {2, 2}, cfg);
  const Series q = agent.predict(stacked).to(pluss::pCPU, ScalarType::Float64);
  const auto qv = q.reshape({4}).to_vec<double>();
  if (!(qv[1] > qv[0] && qv[2] > qv[3])) {
    if (err) {
      *err << "ice ActionValueAgent should score the optimal action highest; got "
           << "[A0=" << qv[0] << ", A1=" << qv[1] << "] "
           << "[B0=" << qv[2] << ", B1=" << qv[3] << "]\n";
    }
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "ice_action_value_agent_roundtrip.bin";
  agent.save(tmp.string());
  const auto restored = ActionValueAgent::load(tmp.string());
  std::filesystem::remove(tmp);
  std::filesystem::remove(tmp.string() + ".online.weights");
  std::filesystem::remove(tmp.string() + ".target.weights");

  const Series q_restored = restored.predict(stacked).to(pluss::pCPU, ScalarType::Float64);
  const auto rv = q_restored.reshape({4}).to_vec<double>();
  for (std::size_t i = 0; i < qv.size(); ++i) {
    if (std::abs(qv[i] - rv[i]) > 1e-9) {
      if (err) *err << "ice ActionValueAgent save/load should preserve q-values exactly\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice ActionValueAgent trains and round-trips", "[ai][ice][rl]") {
  if (!run_ice_action_value_agent_checks(&std::cerr)) {
    FAIL("ice ActionValueAgent checks failed");
  }
}
#else
int main() {
  if (!run_ice_action_value_agent_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "ice ActionValueAgent checks passed\n";
  return 0;
}
#endif
