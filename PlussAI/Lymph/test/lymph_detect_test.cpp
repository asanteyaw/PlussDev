#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
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

FeatureBatch anomaly_batch() {
  std::vector<double> values{};
  values.reserve(52);
  for (int i = 0; i < 24; ++i) {
    const double t = static_cast<double>(i);
    values.push_back(-0.8 + 0.07 * t);
    values.push_back(-0.4 + 0.03 * t);
  }
  values.push_back(5.0);
  values.push_back(5.0);
  values.push_back(-5.0);
  values.push_back(4.0);
  return FeatureBatch{
      .x = pluss::make_series(values, {26, 2}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

DataFrame anomaly_frame() {
  const Series x = anomaly_batch().x;
  return DataFrame({
      {"x0", Thread(x.narrow_copy(1, 0, 1).reshape({26}))},
      {"x1", Thread(x.narrow_copy(1, 1, 1).reshape({26}))},
  });
}

bool run_detect_checks(std::ostream* err) {
  {
    KernelFamilyOptions options{};
    options.kernel = KernelKind::Rbf;
    options.form = KernelForm::OneClass;
    options.gamma = 0.7;
    options.nu = 0.2;

    const detect::Result found = detect::model(models::one_class_svm(options)).run(anomaly_batch());
    if (!(found.has_model()
          && !found.has_scores()
          && found.labels().numel() == 26
          && found.flagged().numel() == 26
          && found.board().ncases() == 4)) {
      if (err) *err << "detect::model(...).run should return labels and summary for one-class models\n";
      return false;
    }
  }

  {
    prep::OutlierIsolatorOptions options{};
    options.estimators = 25;
    options.max_samples_auto = false;
    options.max_samples = 16;
    options.contamination_auto = false;
    options.contamination = 0.12;
    options.random.seed = 19;
    options.random.deterministic = true;

    const detect::Result found = detect::model(models::isolation_detector(options)).run(anomaly_frame());
    if (!(found.has_model()
          && !found.has_scores()
          && found.labels().numel() == 26
          && found.flagged_count() > 0
          && found.board().ncases() == 4)) {
      if (err) *err << "detect::model(models::isolation_detector(...)).run should return labels and summary\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph detect anomaly result family", "[lymph][detect]") {
  REQUIRE(run_detect_checks(&std::cerr));
}
#else
int main() {
  if (!run_detect_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph detect checks passed\n";
  return 0;
}
#endif
