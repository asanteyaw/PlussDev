#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/nn.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <string_view>
#include <string>
#include <vector>

namespace pluss::ai::ice {
class Model;
class Callback;
} // namespace pluss::ai::ice

namespace pluss::ai::ice::io {

// A generic, versioned, checksummed parameter-list artifact — the same
// "walk registered parameters in a fixed order, dump shape + values" idea
// generalized from NodeB's tasks/deep/token_classifier.cpp, but usable by
// any ice::Model rather than being written per call site.
//
// `parameters` must be in the same order, with the same shapes, on both
// save and load — i.e. the caller reconstructs an identical architecture
// before loading, then this restores the trained values into it.
void save_parameters(std::string_view filepath, const std::vector<Series*>& parameters);
void load_parameters(std::string_view filepath, const std::vector<Series*>& parameters);

struct CheckpointState {
  TrainOptions recipe{};
  std::int64_t next_epoch = 0;
  std::int64_t global_step = 0;
  std::optional<double> best_metric = std::nullopt;
  std::string best_metric_name{};
  std::string best_checkpoint_path{};
  std::string last_checkpoint_path{};
  std::string stop_reason{};
  std::string rng_state{};
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> callback_state{};
};

void save_checkpoint(std::string_view filepath,
                     const pluss::ai::ice::Model& model,
                     const pluss::nn::optim::Optimizer& optimizer,
                     const CheckpointState& state,
                     const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks = {});
[[nodiscard]] CheckpointState load_checkpoint(
    std::string_view filepath,
    pluss::ai::ice::Model& model,
    pluss::nn::optim::Optimizer& optimizer,
    const std::vector<std::shared_ptr<pluss::ai::ice::Callback>>& callbacks = {});

} // namespace pluss::ai::ice::io
