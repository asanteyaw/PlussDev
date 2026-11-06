#include "pluss/ai/ice/data/time_series_dataset.h"
#include "pluss/ai/ice/source.h"

#include "pluss/tils.h"

namespace pluss::ai::ice::data {

namespace {

[[nodiscard]] DataSpec to_spec_(const TimeSeriesDataSetOptions& options) {
  DataSpec spec{};
  spec.layout = options.group_ids.empty() ? Layout::TimeSeries : Layout::Panel;
  spec.time_key = options.time_idx;
  spec.group_keys = options.group_ids;
  spec.target = options.target;
  spec.fields = options.variables;
  spec.window = WindowSpec{
      .lookback = options.encoder_length,
      .horizon = options.decoder_length,
  };
  spec.cfg = options.cfg;
  return spec;
}

} // namespace

TimeSeriesBatch::TimeSeriesBatch(const Batch& batch) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(
      batch.layout() == Layout::Panel || batch.layout() == Layout::TimeSeries,
      "pluss::ai::ice::data::TimeSeriesBatch: expected a Panel or TimeSeries batch layout.");
  if (batch.has(field_names::static_inputs)) {
    static_inputs = batch.field(field_names::static_inputs);
    static_size = batch.feature_width(field_names::static_inputs);
    static_categoricals = batch.categoricals(field_names::static_inputs);
  }
  if (batch.has(field_names::encoder_inputs)) {
    encoder_inputs = batch.field(field_names::encoder_inputs);
    encoder_size = batch.feature_width(field_names::encoder_inputs);
    encoder_categoricals = batch.categoricals(field_names::encoder_inputs);
  }
  if (batch.has(field_names::decoder_inputs)) {
    decoder_inputs = batch.field(field_names::decoder_inputs);
    decoder_size = batch.feature_width(field_names::decoder_inputs);
    decoder_categoricals = batch.categoricals(field_names::decoder_inputs);
  }
  if (batch.has(field_names::encoder_target)) {
    encoder_target = batch.field(field_names::encoder_target);
  }
  target = batch.target();
}

Batch TimeSeriesBatch::to_batch() const {
  Batch batch(Layout::Panel);
  if (static_inputs.defined()) {
    batch.set(std::string(field_names::static_inputs), static_inputs, static_categoricals);
  }
  if (encoder_inputs.defined()) {
    batch.set(std::string(field_names::encoder_inputs), encoder_inputs, encoder_categoricals);
  }
  if (decoder_inputs.defined()) {
    batch.set(std::string(field_names::decoder_inputs), decoder_inputs, decoder_categoricals);
  }
  if (encoder_target.defined()) {
    batch.set(std::string(field_names::encoder_target), encoder_target);
  }
  if (target.defined()) {
    batch.set_target(target);
  }
  return batch;
}

TimeSeriesDataSet TimeSeriesDataSet::from_dataframe(const pluss::table::DataFrame& df,
                                                    const TimeSeriesDataSetOptions& options) {
  PLUSS_CALL_GUARD();
  TimeSeriesDataSet out{};
  out.data_ = DataSet::from_frame(df, to_spec_(options));
  out.batch_ = TimeSeriesBatch(out.data_.batch());
  out.vocabularies_ = out.data_.vocabularies();
  return out;
}

TimeSeriesDataSet TimeSeriesDataSet::from_dataframe(const pluss::table::DataFrame& df,
                                                    const TimeSeriesDataSetOptions& options,
                                                    const TimeSeriesDataSet& vocabularies_from) {
  PLUSS_CALL_GUARD();
  TimeSeriesDataSet out{};
  out.data_ = DataSet::from_frame(df, to_spec_(options), vocabularies_from.data_);
  out.batch_ = TimeSeriesBatch(out.data_.batch());
  out.vocabularies_ = out.data_.vocabularies();
  return out;
}

TimeSeriesDataSet TimeSeriesDataSet::from_series(TimeSeriesBatch batch) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(
      batch.encoder_inputs.defined() && batch.decoder_inputs.defined() &&
          batch.encoder_target.defined() && batch.target.defined(),
      "pluss::ai::ice::data::TimeSeriesDataSet::from_series: encoder_inputs, decoder_inputs, "
      "encoder_target and target are required.");
  if (batch.static_size == 0 && batch.static_inputs.defined()) {
    batch.static_size = batch.static_inputs.sizes().back();
  }
  if (batch.encoder_size == 0) {
    batch.encoder_size = batch.encoder_inputs.sizes().back();
  }
  if (batch.decoder_size == 0) {
    batch.decoder_size = batch.decoder_inputs.sizes().back();
  }

  TimeSeriesDataSet out{};
  out.batch_ = std::move(batch);
  out.data_ = DataSet::from_batch(out.batch_.to_batch());
  out.vocabularies_ = out.data_.vocabularies();
  return out;
}

std::int64_t TimeSeriesDataSet::size() const noexcept { return data_.size(); }

std::unique_ptr<pluss::ai::ice::Source> TimeSeriesDataSet::source() const {
  PLUSS_CALL_GUARD();
  return source(pluss::ai::ice::BatchPlan{}, pluss::ai::ice::BatchPlan{});
}

std::unique_ptr<pluss::ai::ice::Source> TimeSeriesDataSet::source(
    const pluss::ai::ice::BatchPlan& learn_plan,
    const pluss::ai::ice::BatchPlan& eval_plan) const {
  PLUSS_CALL_GUARD();
  return data_.source(learn_plan, eval_plan);
}

} // namespace pluss::ai::ice::data
