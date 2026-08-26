#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <cstdint>
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

// y = expm1(2x) is strongly right-skewed and nonlinear in x, but log1p(y) = 2x
// is exactly linear -- the textbook case a target transform exists for: a
// plain linear model badly underfits y directly, the same model with
// .target_transform(transform::log1p()) attached should recover it almost
// exactly.
bool run_target_transform_checks(std::ostream* err) {
  std::vector<double> x_values;
  std::vector<double> y_values;
  for (int i = 0; i <= 20; ++i) {
    const double x = static_cast<double>(i) * 0.15;
    x_values.push_back(x);
    y_values.push_back(std::expm1(2.0 * x));
  }
  const Series x_series = pluss::make_series(x_values, {static_cast<std::int64_t>(x_values.size())}, f64());
  const Series y_series = pluss::make_series(y_values, {static_cast<std::int64_t>(y_values.size())}, f64());
  const DataFrame x_frame({{"x", Thread(x_series)}});

  // 1) A plain linear model fit directly on y should do a visibly poor job --
  // establishes the transform is actually earning its keep, not just plumbing.
  models::Model plain(linear_spec());
  plain.learn_(x_frame, y_series);
  const MetricReport plain_report = plain.assess(x_frame, y_series);
  const double plain_r2 = plain_report.r2.item<double>();

  // 2) The same model family, chained with .target_transform(transform::
  // log1p()), should fit (in original y units) far better on this data.
  // has_target_transform()/.target_transform() (getter) should reflect it
  // before and after set.
  models::Model wrapped(linear_spec());
  if (wrapped.has_target_transform()) {
    if (err) *err << "a freshly-built Model should not have a target_transform set\n";
    return false;
  }
  wrapped.target_transform(transform::log1p());
  if (!wrapped.has_target_transform() || wrapped.target_transform().name != "log1p") {
    if (err) *err << "Model::target_transform(...) should set and expose the transform\n";
    return false;
  }
  wrapped.learn_(x_frame, y_series);
  const Series predicted = wrapped.infer(x_frame);
  const auto predicted_values = as_f64(predicted);
  if (predicted_values.size() != y_values.size()) {
    if (err) *err << "Model::infer with a target_transform attached should return one prediction per row\n";
    return false;
  }
  double max_abs_err = 0.0;
  for (std::size_t i = 0; i < y_values.size(); ++i) {
    max_abs_err = std::max(max_abs_err, std::abs(predicted_values[i] - y_values[i]));
  }
  if (max_abs_err > 0.05) {
    if (err) *err << "Model::infer should recover y almost exactly on noiseless log1p-linear data, "
                  << "max abs error was " << max_abs_err << "\n";
    return false;
  }

  const MetricReport wrapped_report = wrapped.assess(x_frame, y_series);
  const double wrapped_r2 = wrapped_report.r2.item<double>();
  if (wrapped_r2 < 0.999) {
    if (err) *err << "Model::assess should score close to R2=1 in original y units, got "
                  << wrapped_r2 << "\n";
    return false;
  }
  if (!(wrapped_r2 > plain_r2)) {
    if (err) *err << "target_transform(log1p()) should out-score the same model fit directly on y "
                  << "(wrapped R2=" << wrapped_r2 << ", plain R2=" << plain_r2 << ")\n";
    return false;
  }

  // 3) target_transform(...) must reject a classification model outright
  // (fail fast, at set time).
  {
    bool threw = false;
    try {
      models::Model classifier(ModelSpec{
          .task = TaskKind::Classification,
          .family = ModelFamily::Linear,
          .options = LinearFamilyOptions{.objective = LinearObjective::Logistic, .fit_intercept = true},
      });
      classifier.target_transform(transform::log1p());
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Model::target_transform should reject a non-regression task_kind\n";
      return false;
    }
  }

  // 4) check_inverse should catch a transform whose forward/inverse do not
  // actually invert each other, instead of silently training against it.
  {
    bool threw = false;
    try {
      models::Model bad(linear_spec());
      bad.target_transform(transform::custom(
          "broken",
          [](const Series& y) { return y.mul(2.0); },
          [](const Series& y) { return y.mul(3.0); }));  // wrong: should be div(2.0)
      bad.learn_(x_frame, y_series);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Model should reject a transform whose forward/inverse do not round-trip\n";
      return false;
    }
  }

  // check_inverse = false should bypass that guard for a caller who knows
  // what they are doing (e.g. a transform only invertible over part of the range).
  {
    models::Model bypass(linear_spec());
    bypass.target_transform(transform::custom(
        "broken_but_unchecked",
        [](const Series& y) { return y.mul(2.0); },
        [](const Series& y) { return y.mul(3.0); },
        /*check_inverse=*/false));
    bypass.learn_(x_frame, y_series);  // should not throw
  }

  // 5) save() must refuse a model with a target_transform attached --
  // forward/inverse closures are not serializable, so this must fail loudly
  // rather than silently drop the transform on a save/load round trip.
  {
    bool threw = false;
    try {
      wrapped.save("/tmp/lymph_target_transform_test_should_not_exist.bin", x_frame, y_series);
    } catch (const std::exception&) {
      threw = true;
    }
    if (!threw) {
      if (err) *err << "Model::save should refuse to persist a model with a target_transform attached\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph Model::target_transform", "[lymph][target_transform]") {
  if (!run_target_transform_checks(&std::cerr)) {
    FAIL("lymph target_transform checks failed");
  }
}
#else
int main() {
  if (!run_target_transform_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph target_transform checks passed\n";
  return 0;
}
#endif
