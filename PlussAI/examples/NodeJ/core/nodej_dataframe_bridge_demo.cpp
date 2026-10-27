#include <pluss/ml.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

template <class T>
bool require_ok(const StatusOr<T>& out, const std::string& label) {
  if (out.ok()) return true;
  std::cerr << label << " failed: " << out.status().message << "\n";
  return false;
}

void print_metrics(const std::string& label, const Series& metrics) {
  const auto v = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()}).host<double>();
  std::cout << label << ":";
  for (double m : v) std::cout << " " << m;
  std::cout << "\n";
}

DataFrame make_frame() {
  const Series x0 = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
  const Series x1 = pluss::make_series(std::vector<double>{1.0, 0.0, 2.0, 1.0, 3.0, 2.0}, {6}, f64());
  const auto category = pluss::make_note({"red", "blue", "red", "green", "blue", "red"});
  const Series epoch_nanos = pluss::make_series(
      std::vector<std::int64_t>{
          1'700'000'000'000'000'000LL, 1'700'000'001'000'000'000LL, 1'700'000'002'000'000'000LL,
          1'700'000'003'000'000'000LL, 1'700'000'004'000'000'000LL, 1'700'000'005'000'000'000LL},
      {6},
      Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));
  const Slate ts = Slate(epoch_nanos).cast().timeline();
  const Series y = x0.mul(2.0).sub(x1.mul(0.5)).add(1.0);
  const Series w = pluss::make_series(std::vector<double>{1.0, 1.0, 0.8, 1.2, 1.0, 0.9}, {6}, f64());

  return DataFrame({
      {"x0", Slate(x0)},
      {"x1", Slate(x1)},
      {"category", Slate(category)},
      {"ts", ts},
      {"target", Slate(y)},
      {"weight", Slate(w)},
  });
}

} // namespace

int main() {
  std::cout << "NodeJ DataFrame bridge demo\n";

  const DataFrame frame = make_frame();

  DataFrameBatchOptions options{};
  options.feature_names = {"x0", "x1", "category", "ts", "target", "weight"};
  options.label_feature = "target";
  options.sample_weight_feature = "weight";
  options.note_policy = DataNotePolicy::Hash;
  options.slate_policy = DataSlatePolicy::CastTemporalToSeries;

  auto batch_or = labeled_batch_from_dataframe(frame, options);
  if (!require_ok(batch_or, "labeled_batch_from_dataframe")) return 1;

  const LabeledBatch& batch = batch_or.value();
  std::cout << "adapted batch: samples=" << batch.features.sample_count()
            << " features=" << batch.features.feature_count()
            << " weighted=" << (batch.sample_weights.defined() ? "yes" : "no") << "\n";

  ModelSpec spec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-8,
          .fit_intercept = true,
      },
  };

  auto reg_or = make_regressor(spec);
  if (!require_ok(reg_or, "make_regressor")) return 1;
  auto reg = reg_or.move_value();
  Status st = reg->learn(batch);
  if (!st.ok()) {
    std::cerr << "learn failed: " << st.message << "\n";
    return 1;
  }

  auto metrics_or = reg->assess(batch, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!require_ok(metrics_or, "assess")) return 1;
  print_metrics("linear metrics [mse, r2]", metrics_or.value());

  std::cout << "NodeJ DataFrame bridge demo completed.\n";
  return 0;
}
