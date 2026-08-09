#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
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

FeatureBatch shaped_batch() {
  std::vector<double> values{};
  values.reserve(80);
  for (int i = 0; i < 40; ++i) {
    const double t = static_cast<double>(i);
    values.push_back(std::exp(t / 12.0));
    values.push_back(-2.0 + 0.1 * t);
  }
  return FeatureBatch{
      .x = pluss::make_series(values, {40, 2}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

DataFrame shaped_frame() {
  const Series x = shaped_batch().x;
  return DataFrame({
      {"skewed", Thread(x.narrow_copy(1, 0, 1).reshape({40}))},
      {"linear", Thread(x.narrow_copy(1, 1, 1).reshape({40}))},
  });
}

bool run_shape_checks(std::ostream* err) {
  {
    shape::ProfileOptions options{};
    options.skew_threshold = 0.25;
    options.heavy_tail_threshold = 10.0;

    const shape::Result profiled = shape::data(shaped_batch(), {"skewed", "linear"}).run(options);
    if (!(profiled.sample_count() == 40 && profiled.feature_count() == 2
          && profiled.mean().numel() == 2
          && profiled.skewness().numel() == 2
          && profiled.gaussianize_candidate().numel() == 2
          && profiled.board().ncases() == 2)) {
      if (err) *err << "shape::data(FeatureBatch).run should return one profile row per feature\n";
      return false;
    }

    const shape::Plan plan = profiled.plan(options);
    if (plan.empty()) {
      if (err) *err << "shape::Result::plan should select at least one skewed feature\n";
      return false;
    }

    Model model = models::logistic_regression({.fit_intercept = true});
    plan.apply(model);
    const DataFrame frame = shaped_frame();
    const DataFrameBatchOptions data_options{.label_feature = "y"};
    (void)data_options;
    if (model.stage_count() != 0) {
      if (err) *err << "shape plan stages should remain pending until frame-backed learning resolves routes\n";
      return false;
    }
  }

  {
    const shape::Result profiled = shape::data(shaped_frame()).run();
    if (!(profiled.feature_count() == 2 && profiled.features()[0] == "skewed")) {
      if (err) *err << "shape::data(DataFrame).run should preserve DataFrame feature names\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph shape profile and plan family", "[lymph][shape]") {
  REQUIRE(run_shape_checks(&std::cerr));
}
#else
int main() {
  if (!run_shape_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph shape checks passed\n";
  return 0;
}
#endif
