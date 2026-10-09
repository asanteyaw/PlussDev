#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
namespace ice = pluss::ai::ice;
namespace data = pluss::ai::ice::data;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

DataFrame make_cross_section_frame() {
  std::vector<float> score_a{};
  std::vector<float> score_b{};
  std::vector<float> label{};
  score_a.reserve(240);
  score_b.reserve(240);
  label.reserve(240);
  for (std::int64_t i = 0; i < 240; ++i) {
    const float a = static_cast<float>((i % 40) - 20) / 5.0F;
    const float b = static_cast<float>(((i * 3) % 50) - 25) / 6.0F;
    score_a.push_back(a);
    score_b.push_back(b);
    label.push_back((1.8F * a - 1.2F * b) > 0.0F ? 1.0F : 0.0F);
  }
  return DataFrame({
      {"score_a", Slate(pluss::make_series(score_a, {static_cast<std::int64_t>(score_a.size())}, float_cfg()))},
      {"score_b", Slate(pluss::make_series(score_b, {static_cast<std::int64_t>(score_b.size())}, float_cfg()))},
      {"label", Slate(pluss::make_series(label, {static_cast<std::int64_t>(label.size())}, float_cfg()))},
  });
}

data::DataSet make_cross_section_dataset() {
  data::DataSpec spec{};
  spec.layout = data::Layout::CrossSection;
  spec.target = "label";
  spec.fields = {
      {"score_a", data::Role::FeatureContinuous},
      {"score_b", data::Role::FeatureContinuous},
  };
  spec.cfg = float_cfg();
  return data::DataSet::from_frame(make_cross_section_frame(), spec);
}

DataFrame make_panel_frame(std::int64_t n_series, std::int64_t steps) {
  std::vector<std::string> group_col{};
  std::vector<float> time_col{};
  std::vector<float> level_col{};
  std::vector<float> position_col{};
  std::vector<float> observed_col{};
  std::vector<float> target_col{};
  group_col.reserve(static_cast<std::size_t>(n_series * steps));
  time_col.reserve(static_cast<std::size_t>(n_series * steps));
  level_col.reserve(static_cast<std::size_t>(n_series * steps));
  position_col.reserve(static_cast<std::size_t>(n_series * steps));
  observed_col.reserve(static_cast<std::size_t>(n_series * steps));
  target_col.reserve(static_cast<std::size_t>(n_series * steps));

  for (std::int64_t g = 0; g < n_series; ++g) {
    const float level = static_cast<float>(g + 1) * 0.5F;
    for (std::int64_t t = 0; t < steps; ++t) {
      const float position = static_cast<float>(t) / static_cast<float>(steps - 1);
      group_col.push_back("series_" + std::to_string(g));
      time_col.push_back(static_cast<float>(t));
      level_col.push_back(level);
      position_col.push_back(position);
      observed_col.push_back(level + 0.01F * static_cast<float>(t));
      target_col.push_back(level + 0.25F * position);
    }
  }

  return DataFrame({
      {"series", Slate(pluss::make_note(group_col))},
      {"t", Slate(pluss::make_series(time_col, {static_cast<std::int64_t>(time_col.size())}, float_cfg()))},
      {"level", Slate(pluss::make_series(level_col, {static_cast<std::int64_t>(level_col.size())}, float_cfg()))},
      {"position",
       Slate(pluss::make_series(position_col, {static_cast<std::int64_t>(position_col.size())}, float_cfg()))},
      {"observed",
       Slate(pluss::make_series(observed_col, {static_cast<std::int64_t>(observed_col.size())}, float_cfg()))},
      {"y", Slate(pluss::make_series(target_col, {static_cast<std::int64_t>(target_col.size())}, float_cfg()))},
  });
}

data::TimeSeriesDataSet make_time_series_dataset() {
  data::TimeSeriesDataSetOptions options{};
  options.time_idx = "t";
  options.group_ids = {"series"};
  options.target = "y";
  options.variables = {
      {"level", data::Role::StaticContinuous},
      {"position", data::Role::KnownContinuous},
      {"observed", data::Role::ObservedContinuous},
  };
  options.encoder_length = 4;
  options.decoder_length = 2;
  options.cfg = float_cfg();
  return data::TimeSeriesDataSet::from_dataframe(make_panel_frame(6, 9), options);
}

bool run_cross_section_source_checks(std::ostream* err = nullptr) {
  const auto dataset = make_cross_section_dataset();
  auto source = dataset.source(
      ice::BatchPlan{
          .batch_size = 32,
          .shuffle = true,
          .seed = 9,
      },
      ice::BatchPlan{
          .batch_size = 64,
          .shuffle = false,
          .seed = 0,
      });

  source->prepare();
  if (source->learn_batches().size() <= 1) {
    if (err) *err << "DataSet::source() should shard learn batches when batch_size is set\n";
    return false;
  }
  if (source->test_batches().empty() || source->infer_batches().empty()) {
    if (err) *err << "DataSet::source() should expose test/infer views for single-dataset quick use\n";
    return false;
  }

  const auto transferred = source->transfer_batch(
      source->learn_batches().front(),
      ice::SourceContext{
          .recipe = nullptr,
          .stage = ice::StepStage::Learn,
          .target_platform = pluss::pCPU,
      });
  if (!transferred.has_inputs() || !transferred.has_targets()) {
    if (err) *err << "Cross-section Source transfer should materialize inputs and targets\n";
    return false;
  }
  const auto input_sizes = transferred.inputs().sizes();
  if (input_sizes.size() != 2 || input_sizes[1] != 2) {
    if (err) *err << "Cross-section Source transfer should preserve [batch, feature] inputs\n";
    return false;
  }

  auto model = ice::Outline(pluss::nn::Fusion(2, 12), pluss::nn::ReLU(), pluss::nn::Fusion(12, 1));
  ice::TrainOptions options{};
  options.loss = ice::LossKind::BCEWithLogits;
  options.optimizer = ice::OptimizerKind::Adam;
  options.execution.max_epochs = 120;
  options.batch_size = 32;
  options.learning_rate = 0.04;
  options.validation.split_ratio = 0.0;
  options.seed = 7;

  model.learn(*source, options);
  if (!model.ready()) {
    if (err) *err << "ice::Model::learn(Source&) should mark the model ready\n";
    return false;
  }

  const auto report = model.assess(*source, ice::LossKind::BCEWithLogits);
  if (report.metric_name != "accuracy" || report.metric < 0.95) {
    if (err) {
      *err << "ice::Model::assess(Source&) should learn a simple separable task; got "
           << report.metric_name << "=" << report.metric << "\n";
    }
    return false;
  }

  const Series predictions = model.infer(*source);
  if (predictions.sizes().empty() || predictions.sizes()[0] != dataset.size()) {
    if (err) *err << "ice::Model::infer(Source&) should return one prediction row per dataset row\n";
    return false;
  }

  return true;
}

bool run_tensor_source_precision_cast_checks(std::ostream* err = nullptr) {
  const Series x = pluss::make_series(
      std::vector<float>{1.0F, 2.0F, 3.0F, 4.0F}, std::vector<std::int64_t>{2, 2}, float_cfg());
  const Config int_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const Series y = pluss::make_series(
      std::vector<std::int64_t>{0, 1}, std::vector<std::int64_t>{2, 1}, int_cfg);

  ice::TensorSource source(
      x,
      y,
      std::nullopt,
      std::nullopt,
      ice::BatchPlan{
          .batch_size = 2,
          .shuffle = false,
          .seed = 0,
      });
  source.prepare();
  const auto transferred = source.transfer_batch(
      source.learn_batches().front(),
      ice::SourceContext{
          .recipe = nullptr,
          .stage = ice::StepStage::Learn,
          .target_platform = pluss::pCPU,
          .target_dtype = ScalarType::Float64,
      });

  if (transferred.inputs().scalar_type() != ScalarType::Float64) {
    if (err) *err << "TensorSource should cast floating inputs to the requested execution precision\n";
    return false;
  }
  if (transferred.targets().scalar_type() != ScalarType::Int64) {
    if (err) *err << "TensorSource should preserve integer targets instead of coercing them to floating precision\n";
    return false;
  }
  return true;
}

bool run_time_series_source_checks(std::ostream* err = nullptr) {
  const auto dataset = make_time_series_dataset();
  auto source = dataset.source(
      ice::BatchPlan{
          .batch_size = 3,
          .shuffle = false,
          .seed = 0,
      },
      ice::BatchPlan{
          .batch_size = 2,
          .shuffle = false,
          .seed = 0,
      });

  source->prepare();
  if (source->learn_batches().empty()) {
    if (err) *err << "TimeSeriesDataSet::source() should produce learn batches\n";
    return false;
  }

  const auto transferred = source->transfer_batch(
      source->learn_batches().front(),
      ice::SourceContext{
          .recipe = nullptr,
          .stage = ice::StepStage::Learn,
          .target_platform = pluss::pCPU,
      });
  if (!transferred.has(ice::batch_field_names::static_inputs) ||
      !transferred.has(ice::batch_field_names::encoder_inputs) ||
      !transferred.has(ice::batch_field_names::decoder_inputs) ||
      !transferred.has_targets()) {
    if (err) *err << "TimeSeries Source transfer should preserve named static/encoder/decode/target fields\n";
    return false;
  }

  const auto static_sizes = transferred.field(ice::batch_field_names::static_inputs).sizes();
  const auto encoder_sizes = transferred.field(ice::batch_field_names::encoder_inputs).sizes();
  const auto decoder_sizes = transferred.field(ice::batch_field_names::decoder_inputs).sizes();
  const auto target_sizes = transferred.targets().sizes();
  if (static_sizes.size() != 2 || static_sizes[0] != 3 || static_sizes[1] != 1) {
    if (err) *err << "TimeSeries Source transfer should preserve [batch, static_features] shape\n";
    return false;
  }
  if (encoder_sizes.size() != 3 || encoder_sizes[0] != 3 || encoder_sizes[1] != 4 || encoder_sizes[2] != 2) {
    if (err) *err << "TimeSeries Source transfer should preserve [batch, encoder_steps, encoder_features] shape\n";
    return false;
  }
  if (decoder_sizes.size() != 3 || decoder_sizes[0] != 3 || decoder_sizes[1] != 2 || decoder_sizes[2] != 1) {
    if (err) *err << "TimeSeries Source transfer should preserve [batch, decoder_steps, decoder_features] shape\n";
    return false;
  }
  if (target_sizes.size() != 2 || target_sizes[0] != 3 || target_sizes[1] != 2) {
    if (err) *err << "TimeSeries Source transfer should preserve [batch, horizon] targets\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice source substrate adapts datasets into coach/model workflows", "[ai][ice][source]") {
  REQUIRE(run_cross_section_source_checks());
  REQUIRE(run_tensor_source_precision_cast_checks());
  REQUIRE(run_time_series_source_checks());
}
#else
int main() {
  bool ok = true;
  ok = run_cross_section_source_checks(&std::cerr) && ok;
  ok = run_tensor_source_precision_cast_checks(&std::cerr) && ok;
  ok = run_time_series_source_checks(&std::cerr) && ok;
  if (!ok) return 1;
  std::cout << "ice_source_test passed\n";
  return 0;
}
#endif
