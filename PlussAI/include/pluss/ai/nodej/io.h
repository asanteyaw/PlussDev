#pragma once

#include "pluss/ai/nodej/compose.h"

#include <cstdint>
#include <string>
#include <utility>

namespace pluss::ai::nodej::io {

// Bumped 4 -> 5 to add FlowBundle::category_maps. load() rejects any
// mismatched schema_version outright (no partial/tolerant reading of older
// formats), so bundles saved before this change need re-saving.
inline constexpr std::uint32_t k_flow_bundle_schema = 5U;

struct FlowBundleMetadata {
  std::uint32_t schema_version = k_flow_bundle_schema;
  std::string producer = "pluss.ai.classic";
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
  // Category maps fitted for any DataNotePolicy::Factorize column the
  // train_batch's DataFrame conversion touched -- empty when Factorize
  // wasn't used. Needed so a restored model keeps encoding raw Note
  // columns with the exact same codes it was trained on.
  CategoryMaps category_maps{};
};

Status save(const std::string& path,
            const Flow& flow,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            FlowBundleMetadata metadata = {},
            const CategoryMaps& category_maps = {});

inline Status save(const std::string& path,
                   const Flow& flow,
                   const TrainOptions& train_options = {},
                   FlowBundleMetadata metadata = {},
                   const CategoryMaps& category_maps = {}) {
  return save(path, flow, LabeledBatch{}, train_options, std::move(metadata), category_maps);
}

StatusOr<FlowBundle> load(const std::string& path);
StatusOr<Flow> restore(const FlowBundle& bundle);
StatusOr<Flow> load_and_restore(const std::string& path);

} // namespace pluss::ai::nodej::io
