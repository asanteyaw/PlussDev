#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

ModelSpec linear_spec() {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::None,
          .fit_intercept = true,
      },
  };
}

// A raw frame with an "id" column that must never reach the model and a
// couple of NaN feature rows that must not reach it either -- the two things
// Policy exists to strip on every learn_/infer/save call, replayably.
DataFrame make_raw_frame(int n) {
  std::vector<double> id_values;
  std::vector<double> x_values;
  std::vector<double> y_values;
  for (int i = 0; i < n; ++i) {
    id_values.push_back(1000.0 + i);
    const double x = static_cast<double>(i) * 0.4;
    if (i == 2 || i == 5) {
      x_values.push_back(std::numeric_limits<double>::quiet_NaN());
    } else {
      x_values.push_back(x);
    }
    y_values.push_back(3.0 * x + 1.0);
  }
  const Series id_series = pluss::make_series(id_values, {static_cast<std::int64_t>(n)}, f64());
  const Series x_series = pluss::make_series(x_values, {static_cast<std::int64_t>(n)}, f64());
  const Series y_series = pluss::make_series(y_values, {static_cast<std::int64_t>(n)}, f64());
  return DataFrame({
      {"id", Thread(id_series)},
      {"x", Thread(x_series)},
      {"y", Thread(y_series)},
  });
}

DataFrameBatchOptions xy_data_options() {
  DataFrameBatchOptions options{};
  options.feature_names = {"x"};
  options.label_feature = "y";
  return options;
}

bool run_workflow_checks(std::ostream* err) {
  const DataFrame raw = make_raw_frame(12);

  // 1) Policy alone: "id" gone, NaN rows gone, "x"/"y" untouched.
  Policy cleanup = policy({.drop_columns = {"id"}, .filter_nulls = true, .null_filter_columns = {"x"}});
  const DataFrame cleaned = cleanup.apply(raw);
  if (cleaned.ncases() != 10) {
    if (err) *err << "Policy::apply should drop exactly the 2 NaN-x rows, got " << cleaned.ncases() << " rows\n";
    return false;
  }
  const auto cleaned_names = cleaned.feataxis().series().features();
  for (const auto& name : cleaned_names) {
    if (name == "id") {
      if (err) *err << "Policy::apply should have dropped the \"id\" column\n";
      return false;
    }
  }

  // 2) Workflow: raw frame in (with "id" and NaNs), correct predictions out.
  pluss::ai::lymph::Workflow deployed = workflow::from(Model(linear_spec()), cleanup);
  deployed.learn_(raw, xy_data_options());
  const Series predicted = deployed.infer(raw.drop({"y"}));
  const auto predicted_values = as_f64(predicted);
  if (predicted_values.size() != 10) {
    if (err) *err << "Workflow::infer should return one prediction per surviving row (10), got "
                  << predicted_values.size() << "\n";
    return false;
  }

  const MetricReport report = deployed.assess(raw, xy_data_options());
  const double r2 = report.r2.item<double>();
  if (r2 < 0.999) {
    if (err) *err << "Workflow::assess should score close to R2=1 on noiseless linear data, got " << r2 << "\n";
    return false;
  }

  // 3) Save/load round-trip: a fresh Workflow loaded from disk should
  // apply the exact same policy and reproduce the same predictions.
  const std::string path = (std::filesystem::temp_directory_path() / "lymph_workflow_test_model.bin").string();
  deployed.save(path, raw, xy_data_options());

  pluss::ai::lymph::Workflow loaded = workflow::load(path);
  if (loaded.policy().drop_columns != cleanup.drop_columns) {
    if (err) *err << "workflow::load should restore drop_columns from the sidecar\n";
    return false;
  }
  if (loaded.policy().null_filter_columns != cleanup.null_filter_columns || !loaded.policy().filter_nulls) {
    if (err) *err << "workflow::load should restore the null-filter policy from the sidecar\n";
    return false;
  }
  const Series reloaded_predicted = loaded.infer(raw.drop({"y"}));
  const auto reloaded_values = as_f64(reloaded_predicted);
  if (reloaded_values.size() != predicted_values.size()) {
    if (err) *err << "Reloaded Workflow should predict the same row count as before saving\n";
    return false;
  }
  for (std::size_t i = 0; i < predicted_values.size(); ++i) {
    if (std::abs(reloaded_values[i] - predicted_values[i]) > 1e-6) {
      if (err) *err << "Reloaded Workflow prediction " << i << " diverged from the pre-save prediction\n";
      return false;
    }
  }

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Workflow / Policy", "[lymph][workflow]") {
  if (!run_workflow_checks(&std::cerr)) {
    FAIL("lymph Workflow / Policy checks failed");
  }
}
#else
int main() {
  if (!run_workflow_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph Workflow / Policy checks passed\n";
  return 0;
}
#endif
