#pragma once

#include "pluss/ai/ice/data/dataset.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pluss::table {
class DataFrame;
} // namespace pluss::table

namespace pluss::ai::ice::data {

using VariableKind = Role;
using VariableSpec = FieldSpec;

struct TimeSeriesBatch {
  Series static_inputs = pluss::series::undefined();
  Series encoder_inputs = pluss::series::undefined();
  Series decoder_inputs = pluss::series::undefined();
  Series encoder_target = pluss::series::undefined();
  Series target = pluss::series::undefined();

  std::int64_t static_size = 0;
  std::int64_t encoder_size = 0;
  std::int64_t decoder_size = 0;
  std::vector<CategoricalVariable> static_categoricals = {};
  std::vector<CategoricalVariable> encoder_categoricals = {};
  std::vector<CategoricalVariable> decoder_categoricals = {};

  TimeSeriesBatch() = default;
  explicit TimeSeriesBatch(const Batch& batch);

  [[nodiscard]] Batch to_batch() const;
};

struct TimeSeriesDataSetOptions {
  std::string time_idx;
  std::vector<std::string> group_ids;
  std::string target;
  std::vector<VariableSpec> variables;
  std::int64_t encoder_length = 0;
  std::int64_t decoder_length = 0;
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
};

class TimeSeriesDataSet {
public:
  [[nodiscard]] static TimeSeriesDataSet from_dataframe(const pluss::table::DataFrame& df,
                                                        const TimeSeriesDataSetOptions& options);
  [[nodiscard]] static TimeSeriesDataSet from_dataframe(const pluss::table::DataFrame& df,
                                                        const TimeSeriesDataSetOptions& options,
                                                        const TimeSeriesDataSet& vocabularies_from);
  [[nodiscard]] static TimeSeriesDataSet from_series(TimeSeriesBatch batch);

  [[nodiscard]] const TimeSeriesBatch& batch() const noexcept { return batch_; }
  [[nodiscard]] const DataSet& data() const noexcept { return data_; }
  [[nodiscard]] std::int64_t size() const noexcept;
  [[nodiscard]] std::unique_ptr<pluss::ai::ice::Source> source() const;
  [[nodiscard]] std::unique_ptr<pluss::ai::ice::Source> source(const pluss::ai::ice::BatchPlan& learn_plan,
                                                                const pluss::ai::ice::BatchPlan& eval_plan) const;
  [[nodiscard]] const std::vector<CategoricalVocabulary>& vocabularies() const noexcept {
    return vocabularies_;
  }

private:
  DataSet data_{};
  TimeSeriesBatch batch_{};
  std::vector<CategoricalVocabulary> vocabularies_{};
};

} // namespace pluss::ai::ice::data
