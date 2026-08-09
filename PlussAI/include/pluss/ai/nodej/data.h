#pragma once

#include "pluss/ai/nodej/dataset.h"
#include "pluss/ai/nodej/options.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
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
  Factorize = 3,
};

// codes[i] == the string that was assigned integer code i (0-indexed) when
// a Note column was factorized. code (categories.size()) is reserved for
// "category not seen when this map was fit" under
// UnknownCategoryPolicy::MissingBucket.
struct CategoryMap {
  std::vector<std::string> categories{};
};

// DataFrame column name -> its fitted CategoryMap. Populated by fitting
// (DataNotePolicy::Factorize with no existing map for that column) and
// consumed by reuse (Factorize with an existing map for that column) --
// see feature_batch_from_dataframe below. Callers that need train/inference
// consistency (for example Lymph's Model) capture the fitted maps at learn() time and
// pass them back in as `existing_categories` on every later infer() call,
// the same way `feature_names_` is already cached and reused.
using CategoryMaps = std::unordered_map<std::string, CategoryMap>;

struct DataFrameBatchOptions {
  std::vector<std::string> feature_names{};
  std::optional<std::string> label_feature{std::nullopt};
  std::optional<std::string> sample_weight_feature{std::nullopt};
  ObservationLayout layout{ObservationLayout::Rows};
  DataThreadPolicy thread_policy{DataThreadPolicy::CastTemporalToSeries};
  DataNotePolicy note_policy{DataNotePolicy::Reject};
  std::uint64_t note_hash_seed{1469598103934665603ULL};
  // Only consulted for DataNotePolicy::Factorize, and only when a category
  // is encountered that isn't in the map being used (fresh-fit columns
  // can't hit this -- everything they see gets a code by construction; a
  // reused map can, for a category absent at fit time).
  UnknownCategoryPolicy factorize_unknown_categorical{UnknownCategoryPolicy::MissingBucket};
};

// `existing_categories`: pass the maps captured from a prior fit (see
// `fitted_categories_out` below) to reuse those exact codes instead of
// refitting -- required for any Factorize column to encode consistently
// between training and inference. Leave null to always fit fresh (the
// original, still-default, stateless behavior).
//
// `fitted_categories_out`: if non-null, every Factorize column that didn't
// have an entry in `existing_categories` gets its freshly-fit CategoryMap
// written here, keyed by column name -- capture this at learn() time.
StatusOr<FeatureBatch> feature_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options = {},
                                                    const CategoryMaps* existing_categories = nullptr,
                                                    CategoryMaps* fitted_categories_out = nullptr);

StatusOr<LabeledBatch> labeled_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options,
                                                    const CategoryMaps* existing_categories = nullptr,
                                                    CategoryMaps* fitted_categories_out = nullptr);

} // namespace pluss::ai::nodej
