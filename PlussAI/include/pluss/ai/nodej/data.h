#pragma once

#include "pluss/ai/nodej/dataset.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pluss::table {
class DataFrame;
}

namespace pluss::ai::nodej {

enum class DataThreadPolicy : std::uint8_t {
  SeriesOnly = 0,
  CastTemporalToSeries = 1,
};

enum class DataNotePolicy : std::uint8_t {
  Reject = 0,
  Drop = 1,
  Hash = 2,
};

struct DataFrameBatchOptions {
  std::vector<std::string> feature_names{};
  std::optional<std::string> label_feature{std::nullopt};
  std::optional<std::string> sample_weight_feature{std::nullopt};
  ObservationLayout layout{ObservationLayout::Rows};
  DataThreadPolicy thread_policy{DataThreadPolicy::CastTemporalToSeries};
  DataNotePolicy note_policy{DataNotePolicy::Reject};
  std::uint64_t note_hash_seed{1469598103934665603ULL};
};

StatusOr<FeatureBatch> feature_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options = {});

StatusOr<LabeledBatch> labeled_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options);

} // namespace pluss::ai::nodej
