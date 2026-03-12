#include <pluss/ai/ml.h>

#include <cmath>
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
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::InferOptions;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::UnknownCategoryPolicy;
namespace prep = pluss::ai::nodej::prep;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_prep_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            3.0, 4.0,
                                            0.0, 0.0},
                                        {2, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::VectorNormalizer normalizer(prep::VectorNormalizerOptions{
        .norm = prep::VectorNorm::L2,
    });
    auto out_or = normalizer.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "VectorNormalizer learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], 0.6, 1e-9) || !approx(out[1], 0.8, 1e-9) || !approx(out[2], 0.0, 1e-9) || !approx(out[3], 0.0, 1e-9)) {
      if (err) *err << "VectorNormalizer should normalize each sample by configured norm\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 1.0,
                                            4.0, -3.0},
                                        {2, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::MaxAbsScaler scaler;
    auto out_or = scaler.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "MaxAbsScaler learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], -0.5, 1e-9) || !approx(out[1], 1.0 / 3.0, 1e-9) || !approx(out[2], 1.0, 1e-9)
        || !approx(out[3], -1.0, 1e-9)) {
      if (err) *err << "MaxAbsScaler should divide each feature by max absolute value\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4, 1}, f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::QuantileMap uniform(prep::QuantileMapOptions{
        .output = prep::QuantileMapOutput::Uniform,
        .clip = 1e-7,
    });
    auto uniform_or = uniform.learn_map(batch);
    if (!uniform_or.ok()) {
      if (err) *err << "QuantileMap(uniform) learn_map should succeed\n";
      return false;
    }
    const auto u = vec_f64(uniform_or.value());
    if (!(u[0] < u[1] && u[1] < u[2] && u[2] < u[3])) {
      if (err) *err << "QuantileMap(uniform) should preserve rank order\n";
      return false;
    }

    prep::QuantileMap gaussian(prep::QuantileMapOptions{
        .output = prep::QuantileMapOutput::Gaussian,
        .clip = 1e-4,
    });
    auto gauss_or = gaussian.learn_map(batch);
    if (!gauss_or.ok()) {
      if (err) *err << "QuantileMap(gaussian) learn_map should succeed\n";
      return false;
    }
    const auto g = vec_f64(gauss_or.value());
    if (!(std::isfinite(g[0]) && std::isfinite(g[1]) && std::isfinite(g[2]) && std::isfinite(g[3]) && g[0] < g[3])) {
      if (err) *err << "QuantileMap(gaussian) should emit finite monotonic values\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.5,
                                            -1.0, 1.5,
                                            0.0, 2.5},
                                        {3, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::PowerMap map(prep::PowerMapOptions{
        .mode = prep::PowerMapMode::YeoJohnson,
        .standardize = false,
        .lambda = 1.0,
    });
    auto out_or = map.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "PowerMap learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    const auto in = vec_f64(x);
    if (out.size() != in.size()) {
      if (err) *err << "PowerMap output size should match input\n";
      return false;
    }
    for (std::size_t i = 0; i < out.size(); ++i) {
      if (!approx(out[i], in[i], 1e-9)) {
        if (err) *err << "PowerMap with lambda=1 and no standardization should be identity\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.2, 0.7, 0.5, 1.2}, {4, 1}, f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::ThresholdBinarizer bin(prep::ThresholdBinarizerOptions{
        .threshold = 0.5,
        .low_value = 0.0,
        .high_value = 1.0,
    });
    auto out_or = bin.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "ThresholdBinarizer learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], 0.0) || !approx(out[1], 1.0) || !approx(out[2], 0.0) || !approx(out[3], 1.0)) {
      if (err) *err << "ThresholdBinarizer should map values by threshold\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            2.0, 3.0,
                                            1.0, 4.0},
                                        {2, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::PolynomialLift poly(prep::PolynomialLiftOptions{
        .degree = 2,
        .interactions_only = false,
        .include_bias = true,
    });
    auto out_or = poly.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "PolynomialLift learn_map should succeed\n";
      return false;
    }
    const auto sizes = out_or.value().sizes();
    if (sizes.size() != 2 || sizes[0] != 2 || sizes[1] != 6) {
      if (err) *err << "PolynomialLift degree-2 with bias should emit 6 features for 2 inputs\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], 1.0) || !approx(out[1], 2.0) || !approx(out[2], 3.0) || !approx(out[3], 4.0)
        || !approx(out[4], 6.0) || !approx(out[5], 9.0)) {
      if (err) *err << "PolynomialLift first row should match expected polynomial expansion order\n";
      return false;
    }
  }

  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, nan,
                                            2.0, 2.0,
                                            3.0, 3.0},
                                        {3, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::NeighborImputer imp(prep::NeighborImputerOptions{
        .neighbors = 1,
        .weighting = prep::NeighborImputerWeighting::Uniform,
    });
    auto out_or = imp.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "NeighborImputer learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[1], 2.0, 1e-9)) {
      if (err) *err << "NeighborImputer should impute from nearest observed neighbor\n";
      return false;
    }
  }

  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, 2.0,
                                            2.0, 4.0,
                                            3.0, nan,
                                            4.0, 8.0},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::IterativeImputer imp(prep::IterativeImputerOptions{
        .rounds = 6,
        .ridge = 1e-8,
    });
    auto out_or = imp.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "IterativeImputer learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!std::isfinite(out[5]) || std::abs(out[5] - 6.0) > 0.8) {
      if (err) *err << "IterativeImputer should recover linear missing-value trend\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, 10.0,
                                            2.0, 20.0,
                                            3.0, 30.0,
                                            4.0, 40.0},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::StandardScaler scaler;

    auto transformed_a = scaler.learn_map(batch);
    if (!transformed_a.ok()) {
      if (err) *err << "StandardScaler learn_map should succeed\n";
      return false;
    }

    prep::StandardScaler scaler2;
    if (!scaler2.learn(batch).ok()) {
      if (err) *err << "StandardScaler learn should succeed\n";
      return false;
    }
    auto transformed_b = scaler2.map(batch);
    if (!transformed_b.ok()) {
      if (err) *err << "StandardScaler map should succeed after learn\n";
      return false;
    }

    const auto a = vec_f64(transformed_a.value());
    const auto b = vec_f64(transformed_b.value());
    if (a.size() != b.size()) {
      if (err) *err << "StandardScaler transformed vectors should have matching size\n";
      return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
      if (!approx(a[i], b[i], 1e-10)) {
        if (err) *err << "StandardScaler learn_map should match learn+map\n";
        return false;
      }
    }
    if (!approx(a[0], -1.3416407865, 1e-6) || !approx(a[1], -1.3416407865, 1e-6)) {
      if (err) *err << "StandardScaler first row expected z-score values\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, 10.0,
                                            2.0, 20.0,
                                            3.0, 30.0,
                                            4.0, 40.0},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::MinMaxScaler scaler(prep::MinMaxScalerOptions{
        .lower = -1.0,
        .upper = 1.0,
        .clip = true,
    });
    auto out_or = scaler.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "MinMaxScaler learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], -1.0) || !approx(out[1], -1.0) || !approx(out[6], 1.0) || !approx(out[7], 1.0)) {
      if (err) *err << "MinMaxScaler should map endpoints to configured bounds\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, {5, 1}, f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::RobustScaler scaler;
    auto out_or = scaler.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "RobustScaler learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], -1.0, 1e-9) || !approx(out[2], 0.0, 1e-9) || !approx(out[4], 1.0, 1e-9)) {
      if (err) *err << "RobustScaler expected median/IQR normalization on monotonic feature\n";
      return false;
    }
  }

  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, nan,
                                            nan, 2.0,
                                            3.0, 2.0,
                                            nan, nan},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};

    prep::SimpleImputer mean_imp(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean});
    auto mean_or = mean_imp.learn_map(batch);
    if (!mean_or.ok()) {
      if (err) *err << "SimpleImputer(mean) learn_map should succeed\n";
      return false;
    }
    const auto mean_out = vec_f64(mean_or.value());
    if (!approx(mean_out[1], 2.0) || !approx(mean_out[2], 2.0) || !approx(mean_out[7], 2.0)) {
      if (err) *err << "SimpleImputer(mean) should fill NaNs with feature means\n";
      return false;
    }

    prep::SimpleImputer mode_imp(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::MostFrequent});
    auto mode_or = mode_imp.learn_map(batch);
    if (!mode_or.ok()) {
      if (err) *err << "SimpleImputer(most frequent) learn_map should succeed\n";
      return false;
    }
    const auto mode_out = vec_f64(mode_or.value());
    if (!approx(mode_out[2], 1.0) || !approx(mode_out[7], 2.0)) {
      if (err) *err << "SimpleImputer(most frequent) should fill with per-feature modes\n";
      return false;
    }

    prep::SimpleImputer const_imp(prep::SimpleImputerOptions{
        .strategy = prep::SimpleImputerStrategy::Constant,
        .constant_value = -5.0,
    });
    auto const_or = const_imp.learn_map(batch);
    if (!const_or.ok()) {
      if (err) *err << "SimpleImputer(constant) learn_map should succeed\n";
      return false;
    }
    const auto const_out = vec_f64(const_or.value());
    if (!approx(const_out[1], -5.0) || !approx(const_out[2], -5.0) || !approx(const_out[7], -5.0)) {
      if (err) *err << "SimpleImputer(constant) should fill NaNs with configured constant\n";
      return false;
    }
  }

  {
    const Series x_cols = pluss::make_series(std::vector<double>{
                                                 1.0, 2.0, 3.0, 4.0,
                                                 10.0, 20.0, 30.0, 40.0},
                                             {2, 4},
                                             f64());
    const FeatureBatch batch_cols{.x = x_cols, .layout = ObservationLayout::Columns};
    prep::MinMaxScaler scaler;
    auto out_or = scaler.learn_map(batch_cols);
    if (!out_or.ok()) {
      if (err) *err << "MinMaxScaler should support ObservationLayout::Columns\n";
      return false;
    }
    const auto out_sizes = out_or.value().sizes();
    const auto in_sizes = x_cols.sizes();
    if (out_sizes.size() != in_sizes.size() ||
        out_sizes[0] != in_sizes[0] ||
        out_sizes[1] != in_sizes[1]) {
      if (err) *err << "column-layout transform should preserve matrix shape\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, 10.0,
                                            2.0, 10.0,
                                            1.0, 20.0,
                                            3.0, 10.0},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};
    prep::OneHotEncoder enc;

    auto out_a_or = enc.learn_map(batch);
    if (!out_a_or.ok()) {
      if (err) *err << "OneHotEncoder learn_map should succeed\n";
      return false;
    }

    prep::OneHotEncoder enc2;
    if (!enc2.learn(batch).ok()) {
      if (err) *err << "OneHotEncoder learn should succeed\n";
      return false;
    }
    auto out_b_or = enc2.map(batch);
    if (!out_b_or.ok()) {
      if (err) *err << "OneHotEncoder map should succeed after learn\n";
      return false;
    }

    const auto out_a = vec_f64(out_a_or.value());
    const auto out_b = vec_f64(out_b_or.value());
    if (out_a.size() != out_b.size()) {
      if (err) *err << "OneHotEncoder transformed vectors should have matching size\n";
      return false;
    }
    for (std::size_t i = 0; i < out_a.size(); ++i) {
      if (!approx(out_a[i], out_b[i], 1e-10)) {
        if (err) *err << "OneHotEncoder learn_map should match learn+map\n";
        return false;
      }
    }

    const auto sizes = out_a_or.value().sizes();
    if (sizes.size() != 2 || sizes[0] != 4 || sizes[1] != 7) {
      if (err) *err << "OneHotEncoder expected shape [4,7]\n";
      return false;
    }
    if (!approx(out_a[0], 1.0) || !approx(out_a[4], 1.0) || !approx(out_a[8], 1.0) || !approx(out_a[11], 1.0)) {
      if (err) *err << "OneHotEncoder expected deterministic one-hot positions\n";
      return false;
    }

    const Series x_unknown = pluss::make_series(std::vector<double>{4.0, 30.0}, {1, 2}, f64());
    const FeatureBatch unknown_batch{.x = x_unknown, .layout = ObservationLayout::Rows};

    auto err_or = enc2.map(unknown_batch);
    if (err_or.ok()) {
      if (err) *err << "OneHotEncoder default unknown policy should error\n";
      return false;
    }

    InferOptions infer_opts{};
    infer_opts.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    auto missing_or = enc2.map(unknown_batch, infer_opts);
    if (!missing_or.ok()) {
      if (err) *err << "OneHotEncoder should support unknown MissingBucket policy\n";
      return false;
    }
    const auto missing = vec_f64(missing_or.value());
    if (missing.size() != 7 || !approx(missing[3], 1.0) || !approx(missing[6], 1.0)) {
      if (err) *err << "OneHotEncoder missing bucket positions should be activated\n";
      return false;
    }
  }

  {
    const Series x_cols = pluss::make_series(std::vector<double>{
                                                 1.0, 2.0, 1.0, 3.0,
                                                 10.0, 10.0, 20.0, 10.0},
                                             {2, 4},
                                             f64());
    const FeatureBatch batch_cols{.x = x_cols, .layout = ObservationLayout::Columns};
    prep::OneHotEncoder enc;
    auto out_or = enc.learn_map(batch_cols);
    if (!out_or.ok()) {
      if (err) *err << "OneHotEncoder should support ObservationLayout::Columns\n";
      return false;
    }
    const auto sizes = out_or.value().sizes();
    if (sizes.size() != 2 || sizes[0] != 7 || sizes[1] != 4) {
      if (err) *err << "OneHotEncoder column layout should produce [encoded_features,samples]\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            1.0, 10.0,
                                            2.0, 10.0,
                                            1.0, 20.0,
                                            3.0, 10.0},
                                        {4, 2},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};

    prep::FrequencyEncoder enc(prep::FrequencyEncoderOptions{
        .max_categories = 50,
        .overflow = prep::CategoryOverflowPolicy::KeepMostFrequent,
        .unknown_frequency = 0.2,
    });
    auto out_or = enc.learn_map(batch);
    if (!out_or.ok()) {
      if (err) *err << "FrequencyEncoder learn_map should succeed\n";
      return false;
    }
    const auto out = vec_f64(out_or.value());
    if (!approx(out[0], 0.5) || !approx(out[1], 0.75) || !approx(out[2], 0.25) || !approx(out[5], 0.25)) {
      if (err) *err << "FrequencyEncoder should map categories to learned frequencies\n";
      return false;
    }

    const Series x_unknown = pluss::make_series(std::vector<double>{4.0, 30.0}, {1, 2}, f64());
    const FeatureBatch unknown_batch{.x = x_unknown, .layout = ObservationLayout::Rows};

    auto err_or = enc.map(unknown_batch);
    if (err_or.ok()) {
      if (err) *err << "FrequencyEncoder default unknown policy should error\n";
      return false;
    }

    InferOptions infer_opts{};
    infer_opts.unknown_categorical = UnknownCategoryPolicy::MissingBucket;
    auto miss_or = enc.map(unknown_batch, infer_opts);
    if (!miss_or.ok()) {
      if (err) *err << "FrequencyEncoder should support unknown MissingBucket policy\n";
      return false;
    }
    const auto miss = vec_f64(miss_or.value());
    if (miss.size() != 2 || !approx(miss[0], 0.2) || !approx(miss[1], 0.2)) {
      if (err) *err << "FrequencyEncoder should use configured unknown_frequency\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej preprocessors expose learn/map/learn_map behavior") {
  REQUIRE(run_nodej_prep_checks());
}
#else
int main() {
  if (!run_nodej_prep_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_prep_test passed\n";
  return 0;
}
#endif
