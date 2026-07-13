#pragma once

#include "table/DataFrame.h"
#include "pluss/tils/aux/data_loader.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::table {

struct DFDatasetOptions {
  std::vector<std::string> x_features{};
  std::optional<std::string> y_feature{};
  std::optional<std::string> weight_feature{};
  std::vector<std::string> extra_features{};
  std::optional<pluss::ScalarType> dtype{pluss::ScalarType::Float32};
  std::optional<pluss::Platform> platform{};
  std::int64_t batch_dim{0};
};

class DFDataset final : public pluss::loader::Dataset {
public:
  DFDataset() = default;

  explicit DFDataset(DataFrame df, DFDatasetOptions options)
      : df_(std::move(df)),
        options_(std::move(options)),
        dataset_(build_fields_(df_, options_), options_.batch_dim) {}

  [[nodiscard]] std::int64_t size() const override { return dataset_.size(); }
  [[nodiscard]] std::int64_t batch_dim() const noexcept override { return dataset_.batch_dim(); }
  [[nodiscard]] pluss::loader::DataBatch get(std::int64_t index) const override { return dataset_.get(index); }
  [[nodiscard]] std::vector<pluss::Series> materialize_fields() const override {
    return dataset_.materialize_fields();
  }

  [[nodiscard]] const DataFrame& frame() const noexcept { return df_; }
  [[nodiscard]] const DFDatasetOptions& options() const noexcept { return options_; }
  [[nodiscard]] const pluss::loader::SeriesDataset& series_dataset() const noexcept { return dataset_; }

private:
  static pluss::Series cast_column_(const DataFrame& df, std::string_view feature, const DFDatasetOptions& options) {
    PLUSS_CHECK(df.has_feature(feature), "pluss::DFDataset: missing feature '" + std::string(feature) + "'");
    auto out = df.features(feature).cast(ThreadCastTarget::Series).as_series();
    if (options.platform.has_value() || options.dtype.has_value()) {
      const auto platform = options.platform.value_or(out.platform());
      const auto dtype = options.dtype.value_or(out.scalar_type());
      out = out.to(platform, dtype);
    }
    return out;
  }

  static std::vector<std::string> infer_x_features_(const DataFrame& df, const DFDatasetOptions& options) {
    if (!options.x_features.empty()) return options.x_features;

    std::unordered_set<std::string> blocked;
    if (options.y_feature.has_value()) blocked.insert(*options.y_feature);
    if (options.weight_feature.has_value()) blocked.insert(*options.weight_feature);
    for (const auto& name : options.extra_features) blocked.insert(name);

    std::vector<std::string> out;
    for (const auto& name : df.features()) {
      if (blocked.contains(name)) continue;
      if (df.features(name).kind() == ThreadKind::Series) out.push_back(name);
    }
    return out;
  }

  static std::vector<pluss::Series> build_fields_(const DataFrame& df, const DFDatasetOptions& options) {
    PLUSS_CHECK(df.defined(), "pluss::DFDataset: DataFrame is undefined");
    PLUSS_CHECK(df.ncases() > 0, "pluss::DFDataset: DataFrame has no rows");

    const auto x_features = infer_x_features_(df, options);
    PLUSS_CHECK(!x_features.empty(), "pluss::DFDataset: x_features is empty after inference");

    std::vector<pluss::Series> x_cols;
    x_cols.reserve(x_features.size());
    for (const auto& name : x_features) {
      auto col = cast_column_(df, name, options);
      if (col.dim() == 2 && col.sizes()[1] == 1) col = col.squeeze(1);
      PLUSS_CHECK(
          col.dim() == 1,
          "pluss::DFDataset: each x feature must be vector-like (rank 1 or Nx1), got rank=" + std::to_string(col.dim()));
      x_cols.push_back(std::move(col));
    }

    pluss::Series x = (x_cols.size() == 1)
        ? x_cols.front().reshape({df.ncases(), 1})
        : pluss::pile(x_cols, 0).transpose(0, 1);

    std::vector<pluss::Series> fields;
    fields.push_back(std::move(x));

    if (options.y_feature.has_value()) {
      auto y = cast_column_(df, *options.y_feature, options);
      if (y.dim() == 1) y = y.reshape({df.ncases(), 1});
      fields.push_back(std::move(y));
    }

    if (options.weight_feature.has_value()) {
      auto w = cast_column_(df, *options.weight_feature, options);
      if (w.dim() == 1) w = w.reshape({df.ncases(), 1});
      fields.push_back(std::move(w));
    }

    for (const auto& name : options.extra_features) {
      auto extra = cast_column_(df, name, options);
      if (extra.dim() == 1) extra = extra.reshape({df.ncases(), 1});
      fields.push_back(std::move(extra));
    }

    return fields;
  }

  DataFrame df_{};
  DFDatasetOptions options_{};
  pluss::loader::SeriesDataset dataset_{};
};

} // namespace pluss::table

namespace pluss {
using DFDataset = table::DFDataset;
using DFDatasetOptions = table::DFDatasetOptions;
} // namespace pluss
