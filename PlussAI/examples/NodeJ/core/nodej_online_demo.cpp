#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::TrainOptions;
using pluss::ai::nodej::online::OnlineLinearOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

LabeledBatch slice_rows(const LabeledBatch& batch, std::int64_t start, std::int64_t count) {
  const Series idx = pluss::arange(start, start + count, i64());
  const Series x = batch.features.x.index_select(0, idx).reshape({count, batch.features.feature_count()});
  const Series y = batch.y.index_select(0, idx).reshape({count});
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_class_batch() {
  std::vector<double> xv{};
  std::vector<double> yv{};
  xv.reserve(2 * 90);
  yv.reserve(90);
  for (int i = 0; i < 45; ++i) {
    xv.push_back(-2.4 + 0.06 * i);
    xv.push_back(-1.5 + 0.03 * i);
    yv.push_back(0.0);
  }
  for (int i = 0; i < 45; ++i) {
    xv.push_back(1.0 + 0.06 * i);
    xv.push_back(1.4 + 0.03 * i);
    yv.push_back(1.0);
  }
  return LabeledBatch{
      .features = FeatureBatch{.x = pluss::make_series(std::move(xv), {90, 2}, f64()), .layout = ObservationLayout::Rows},
      .y = pluss::make_series(std::move(yv), {90}, f64()),
  };
}

void print_vec(const Series& s) {
  const auto v = s.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < v.size(); ++i) std::cout << v[i] << (i + 1 == v.size() ? '\n' : ' ');
}

} // namespace

int main() {
  const LabeledBatch full = make_class_batch();
  const LabeledBatch warm = slice_rows(full, 36, 18);
  const LabeledBatch stream_a = slice_rows(full, 0, 36);
  const LabeledBatch stream_b = slice_rows(full, 54, 36);

  OnlineLinearOptions online_options{};
  online_options.family = LinearFamilyOptions{
      .objective = LinearObjective::Logistic,
      .regularization = RegularizationKind::L2,
      .regularization_strength = 1e-6,
      .fit_intercept = true,
  };
  online_options.learning_rate = 0.08;
  online_options.decaying_rate = true;

  auto model_or = pluss::ai::nodej::online::make_online_linear_classifier(online_options);
  if (!model_or.ok()) {
    std::cerr << "make_online_linear_classifier failed: " << model_or.status().message << '\n';
    return 1;
  }
  auto model = model_or.move_value();

  TrainOptions warm_opts{};
  warm_opts.max_iterations = 100;
  const auto warm_status = model.learn(warm, warm_opts);
  if (!warm_status.ok()) {
    std::cerr << "learn failed: " << warm_status.message << '\n';
    return 1;
  }

  if (!model.advance(stream_a).ok() || !model.advance(stream_b).ok()) {
    std::cerr << "advance failed\n";
    return 1;
  }

  pluss::ai::nodej::AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::LogLoss};
  auto metric_or = model.assess(full, assess);
  if (!metric_or.ok()) {
    std::cerr << "assess failed: " << metric_or.status().message << '\n';
    return 1;
  }

  std::cout << "online seen_samples=" << model.seen_samples() << '\n';
  std::cout << "online metrics [accuracy, logloss]: ";
  print_vec(metric_or.value());
  return 0;
}
