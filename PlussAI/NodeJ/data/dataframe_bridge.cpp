#include "pluss/ai/nodej/data.h"

#include "pluss/table/table.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] StatusOr<Series> as_numeric_series(const pluss::table::Slate& slate,
                                                 DataSlatePolicy slate_policy,
                                                 DataNotePolicy note_policy,
                                                 std::uint64_t note_hash_seed,
                                                 UnknownCategoryPolicy factorize_unknown_categorical,
                                                 const CategoryMaps* existing_categories,
                                                 CategoryMaps* fitted_categories_out,
                                                 bool allow_note_drop,
                                                 const std::string& name,
                                                 std::int64_t expected_size) {
  if (slate.size() != expected_size) {
    return StatusOr<Series>(
        Status::shape_mismatch("DataFrame feature '" + name + "' size does not match DataFrame case count."));
  }

  if (slate.kind() == pluss::table::SlateKind::Note) {
    if (note_policy == DataNotePolicy::Drop && allow_note_drop) {
      return StatusOr<Series>(pluss::series::undefined());
    }
    if (note_policy == DataNotePolicy::Reject) {
      return StatusOr<Series>(Status::invalid_argument(
          "DataFrame feature '" + name + "' is Note-backed and note_policy is Reject."));
    }
    if (note_policy == DataNotePolicy::Drop) {
      return StatusOr<Series>(Status::invalid_argument(
          "DataFrame feature '" + name + "' cannot be dropped in this adapter context."));
    }

    if (note_policy == DataNotePolicy::Factorize) {
      const CategoryMap* existing_map = nullptr;
      if (existing_categories != nullptr) {
        const auto it = existing_categories->find(name);
        if (it != existing_categories->end()) existing_map = &it->second;
      }

      // Fit fresh (no persisted map for this column) or reuse an already-
      // fitted one -- either way `map_to_use` ends up pointing at the
      // CategoryMap this column's codes are assigned from.
      CategoryMap fitted{};
      const CategoryMap* map_to_use = existing_map;
      if (map_to_use == nullptr) {
        std::set<std::string_view> distinct{};
        for (std::int64_t i = 0; i < expected_size; ++i) {
          if (!pluss::table::detail::valid_at(slate, i)) continue;
          distinct.emplace(slate.as_note().at(i));
        }
        fitted.categories.assign(distinct.begin(), distinct.end());
        map_to_use = &fitted;
      }

      // Views into map_to_use->categories' owned strings, stable for the
      // rest of this function -- avoids allocating a std::string per row
      // just to do a lookup.
      std::unordered_map<std::string_view, std::int64_t> code_of{};
      code_of.reserve(map_to_use->categories.size());
      for (std::size_t i = 0; i < map_to_use->categories.size(); ++i) {
        code_of.emplace(std::string_view(map_to_use->categories[i]), static_cast<std::int64_t>(i));
      }
      const std::int64_t unknown_code = static_cast<std::int64_t>(map_to_use->categories.size());

      auto out = Series::empty({expected_size}, ScalarType::Float64);
      auto* p = reinterpret_cast<double*>(out.raw_data());
      for (std::int64_t i = 0; i < expected_size; ++i) {
        if (!pluss::table::detail::valid_at(slate, i)) {
          p[i] = std::numeric_limits<double>::quiet_NaN();
          continue;
        }
        const std::string_view token = slate.as_note().at(i);
        const auto it = code_of.find(token);
        if (it != code_of.end()) {
          p[i] = static_cast<double>(it->second);
          continue;
        }
        // Only reachable when reusing a persisted map against data whose
        // categories weren't present at fit time -- a freshly-fit map
        // covers everything it just scanned, by construction.
        switch (factorize_unknown_categorical) {
          case UnknownCategoryPolicy::Error:
            return StatusOr<Series>(Status::invalid_argument(
                "DataFrame feature '" + name + "' saw category \"" + std::string(token)
                + "\" that was not present when Factorize was fit."));
          case UnknownCategoryPolicy::MissingBucket:
            p[i] = static_cast<double>(unknown_code);
            break;
          case UnknownCategoryPolicy::Ignore:
            p[i] = std::numeric_limits<double>::quiet_NaN();
            break;
        }
      }

      if (fitted_categories_out != nullptr && existing_map == nullptr) {
        (*fitted_categories_out)[name] = std::move(fitted);
      }
      return StatusOr<Series>(std::move(out));
    }

    auto out = Series::empty({expected_size}, ScalarType::Float64);
    auto* p = reinterpret_cast<double*>(out.raw_data());
    constexpr std::uint64_t fnv_offset = 1469598103934665603ULL;
    constexpr std::uint64_t fnv_prime = 1099511628211ULL;
    for (std::int64_t i = 0; i < expected_size; ++i) {
      if (!pluss::table::detail::valid_at(slate, i)) {
        p[i] = std::numeric_limits<double>::quiet_NaN();
        continue;
      }
      std::uint64_t h = fnv_offset ^ note_hash_seed;
      const std::string_view token = slate.as_note().at(i);
      for (unsigned char ch : token) {
        h ^= static_cast<std::uint64_t>(ch);
        h *= fnv_prime;
      }
      const std::uint32_t lane = static_cast<std::uint32_t>((h ^ (h >> 32)) & 0x7FFFFFFFULL);
      p[i] = static_cast<double>(lane);
    }
    return StatusOr<Series>(std::move(out));
  }

  pluss::table::Slate materialized = slate;
  if (slate.kind() != pluss::table::SlateKind::Series) {
    if (slate_policy == DataSlatePolicy::CastTemporalToSeries &&
        slate.kind() == pluss::table::SlateKind::Timeline) {
      materialized = slate.cast().series();
    } else {
      return StatusOr<Series>(Status::invalid_argument(
          "DataFrame feature '" + name + "' is not a numeric Series column. Cast it before adapter conversion."));
    }
  }

  Series out = materialized.as_series().to(ScalarType::Float64).reshape({expected_size});
  if (pluss::table::detail::has_validity_mask(materialized) &&
      pluss::table::detail::validity_mask(materialized).has_value()) {
    const Series valid = pluss::table::detail::validity_mask(materialized).value()
                             .to(out.platform(), ScalarType::Float64)
                             .reshape({expected_size});
    const Series nan_fill = pluss::series::zeros_like(out, ScalarType::Float64).add(std::numeric_limits<double>::quiet_NaN());
    out = valid.gt(0.5).where(out, nan_fill);
  }
  return StatusOr<Series>(std::move(out));
}

[[nodiscard]] StatusOr<std::vector<std::string>> resolve_feature_names(const pluss::table::DataFrame& frame,
                                                                       const DataFrameBatchOptions& options) {
  if (!options.feature_names.empty()) {
    for (const auto& name : options.feature_names) {
      if (!frame.get(name).has_value()) {
        return StatusOr<std::vector<std::string>>(Status::invalid_argument(
            "DataFrame adapter requested unknown feature '" + name + "'."));
      }
    }
    return StatusOr<std::vector<std::string>>(options.feature_names);
  }

  const auto names = frame.features();
  if (names.empty()) {
    return StatusOr<std::vector<std::string>>(
        Status::invalid_argument("DataFrame adapter requires at least one feature column."));
  }
  return StatusOr<std::vector<std::string>>(names);
}

} // namespace

StatusOr<FeatureBatch> feature_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options,
                                                    const CategoryMaps* existing_categories,
                                                    CategoryMaps* fitted_categories_out) {
  if (!frame.defined()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("DataFrame adapter requires a defined DataFrame."));
  }
  if (frame.ncases() <= 0) {
    return StatusOr<FeatureBatch>(Status::shape_mismatch("DataFrame adapter requires at least one case."));
  }

  StatusOr<std::vector<std::string>> names_or = resolve_feature_names(frame, options);
  if (!names_or.ok()) return StatusOr<FeatureBatch>(names_or.status());
  std::vector<std::string> feature_names = names_or.move_value();

  if (options.label_feature.has_value()) {
    feature_names.erase(
        std::remove(feature_names.begin(), feature_names.end(), *options.label_feature),
        feature_names.end());
  }
  if (options.sample_weight_feature.has_value()) {
    feature_names.erase(
        std::remove(feature_names.begin(), feature_names.end(), *options.sample_weight_feature),
        feature_names.end());
  }
  if (feature_names.empty()) {
    return StatusOr<FeatureBatch>(
        Status::invalid_argument("DataFrame adapter resolved zero feature columns after exclusions."));
  }

  std::vector<Series> columns{};
  columns.reserve(feature_names.size());

  Platform base_platform = pluss::pCPU;
  bool have_platform = false;
  for (const auto& name : feature_names) {
    StatusOr<Series> series_or = as_numeric_series(frame.features(name),
                                                   options.slate_policy,
                                                   options.note_policy,
                                                   options.note_hash_seed,
                                                   options.factorize_unknown_categorical,
                                                   existing_categories,
                                                   fitted_categories_out,
                                                   true,
                                                   name,
                                                   frame.ncases());
    if (!series_or.ok()) return StatusOr<FeatureBatch>(series_or.status());

    Series col = series_or.move_value();
    if (!col.defined()) {
      continue;
    }
    if (!have_platform) {
      base_platform = col.platform();
      have_platform = true;
    }
    if (col.platform() != base_platform) {
      col = col.to(base_platform, ScalarType::Float64);
    }
    columns.push_back(col.reshape({frame.ncases(), 1}));
  }

  if (columns.empty()) {
    return StatusOr<FeatureBatch>(
        Status::invalid_argument("DataFrame adapter resolved zero usable numeric features after routing."));
  }

  Series x_rows = columns.front();
  for (std::size_t i = 1; i < columns.size(); ++i) {
    x_rows = x_rows.unite({columns[i]}, 1);
  }

  FeatureBatch batch{
      .x = options.layout == ObservationLayout::Rows ? x_rows : x_rows.transpose(0, 1),
      .layout = options.layout,
  };
  return StatusOr<FeatureBatch>(std::move(batch));
}

StatusOr<LabeledBatch> labeled_batch_from_dataframe(const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& options,
                                                    const CategoryMaps* existing_categories,
                                                    CategoryMaps* fitted_categories_out) {
  if (!options.label_feature.has_value()) {
    return StatusOr<LabeledBatch>(
        Status::invalid_argument("DataFrame labeled adapter requires DataFrameBatchOptions.label_feature."));
  }
  if (!frame.get(*options.label_feature).has_value()) {
    return StatusOr<LabeledBatch>(Status::invalid_argument(
        "DataFrame labeled adapter cannot find label feature '" + *options.label_feature + "'."));
  }
  if (options.sample_weight_feature.has_value() && !frame.get(*options.sample_weight_feature).has_value()) {
    return StatusOr<LabeledBatch>(Status::invalid_argument(
        "DataFrame labeled adapter cannot find sample-weight feature '" + *options.sample_weight_feature + "'."));
  }

  StatusOr<FeatureBatch> features_or =
      feature_batch_from_dataframe(frame, options, existing_categories, fitted_categories_out);
  if (!features_or.ok()) return StatusOr<LabeledBatch>(features_or.status());

  StatusOr<Series> label_or =
      as_numeric_series(frame.features(*options.label_feature),
                        options.slate_policy,
                        options.note_policy,
                        options.note_hash_seed,
                        options.factorize_unknown_categorical,
                        existing_categories,
                        fitted_categories_out,
                        false,
                        *options.label_feature,
                        frame.ncases());
  if (!label_or.ok()) return StatusOr<LabeledBatch>(label_or.status());
  Series y = label_or.move_value().to(features_or.value().x.platform(), ScalarType::Float64);

  Series sample_weights = pluss::series::undefined();
  if (options.sample_weight_feature.has_value()) {
    StatusOr<Series> weight_or = as_numeric_series(
        frame.features(*options.sample_weight_feature),
        options.slate_policy,
        options.note_policy,
        options.note_hash_seed,
        options.factorize_unknown_categorical,
        existing_categories,
        fitted_categories_out,
        false,
        *options.sample_weight_feature,
        frame.ncases());
    if (!weight_or.ok()) return StatusOr<LabeledBatch>(weight_or.status());
    sample_weights = weight_or.move_value().to(features_or.value().x.platform(), ScalarType::Float64);
  }

  LabeledBatch out{
      .features = features_or.move_value(),
      .y = std::move(y),
      .sample_weights = std::move(sample_weights),
  };
  Status check = validate_labeled(out);
  if (!check.ok()) return StatusOr<LabeledBatch>(std::move(check));
  return StatusOr<LabeledBatch>(std::move(out));
}

} // namespace pluss::ai::nodej
