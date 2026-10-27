#include <pluss/ai/lymph.h>

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
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::TrainOptions;
using pluss::ai::lymph::models::OnlineLinearClassifier;
using pluss::ai::lymph::OnlineLinearOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

LabeledBatch slice_rows(const LabeledBatch& batch, std::int64_t start, std::int64_t count) {
  const Series idx = pluss::series::arange(start, start + count, i64());
  const Series x = batch.features.x.index_select(0, idx).reshape({count, batch.features.feature_count()});
  const Series y = batch.y.index_select(0, idx).reshape({count});
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_batch() {
  std::vector<double> xv{};
  std::vector<double> yv{};
  for (int i = 0; i < 40; ++i) {
    xv.push_back(-2.0 + 0.05 * i);
    xv.push_back(-1.6 + 0.05 * i);
    yv.push_back(0.0);
  }
  for (int i = 0; i < 40; ++i) {
    xv.push_back(1.2 + 0.05 * i);
    xv.push_back(1.0 + 0.05 * i);
    yv.push_back(1.0);
  }
  return LabeledBatch{
      .features = FeatureBatch{.x = pluss::make_series(std::move(xv), {80, 2}, f64()), .layout = ObservationLayout::Rows},
      .y = pluss::make_series(std::move(yv), {80}, f64()),
  };
}

void print_vec(const Series& s) {
  const auto v = s.to(pluss::pCPU, ScalarType::Float64).host<double>();
  for (std::size_t i = 0; i < v.size(); ++i) std::cout << v[i] << (i + 1 == v.size() ? '\n' : ' ');
}

} // namespace

int main() {
  try {
    const LabeledBatch full = make_batch();
    const LabeledBatch warm = slice_rows(full, 30, 20);
    const LabeledBatch stream = slice_rows(full, 20, 60);

    OnlineLinearOptions online_options{};
    online_options.family = LinearFamilyOptions{
        .objective = LinearObjective::Logistic,
        .regularization = RegularizationKind::L2,
        .regularization_strength = 1e-6,
        .fit_intercept = true,
    };
    online_options.learning_rate = 0.08;

    TrainOptions warm_opts{};
    warm_opts.max_iterations = 80;

    OnlineLinearClassifier model = pluss::ai::lymph::models::online::linear_classifier(online_options);
    model.learn_(warm, warm_opts).advance_(stream);

    const auto metrics = model.assess(full);
    const Series metric_values = metrics.accuracy.defined() ? metrics.accuracy : metrics.values;

    std::cout << "lymph online seen_samples=" << model.seen_samples() << '\n';
    std::cout << "lymph online metrics [accuracy]: ";
    print_vec(metric_values);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "lymph_online_demo failed: " << ex.what() << '\n';
    return 1;
  }
}
