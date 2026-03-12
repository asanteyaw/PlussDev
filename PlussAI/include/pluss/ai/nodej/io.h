#pragma once

#include "pluss/ai/nodej/compose.h"

#include <cstdint>
#include <string>

namespace pluss::ai::nodej::io {

inline constexpr std::uint32_t k_flow_bundle_schema = 2U;

struct FlowBundleMetadata {
  std::uint32_t schema_version = k_flow_bundle_schema;
  std::string producer = "pluss.ai.nodej";
  std::string producer_version = "0.1.0";
  std::uint64_t created_unix_nanos = 0;
};

struct FlowBundle {
  FlowBundleMetadata metadata{};
  ModelSpec model{};
  std::vector<TransformSpec> transforms{};
  bool trained = false;
  bool has_snapshot = false;
  std::vector<std::vector<Series>> transform_states{};
  std::vector<Series> model_state{};
  TrainOptions train_options{};
  LabeledBatch train_batch{};
};

Status save(const std::string& path,
            const Flow& flow,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            FlowBundleMetadata metadata = {});

StatusOr<FlowBundle> load(const std::string& path);
StatusOr<Flow> restore(const FlowBundle& bundle);
StatusOr<Flow> load_and_restore(const std::string& path);

} // namespace pluss::ai::nodej::io
