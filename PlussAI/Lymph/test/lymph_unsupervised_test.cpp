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

FeatureBatch cluster_batch() {
  std::vector<double> values{};
  values.reserve(80);
  for (int i = 0; i < 20; ++i) {
    values.push_back(-2.0 + 0.04 * static_cast<double>(i));
    values.push_back(-1.5 + 0.03 * static_cast<double>(i));
  }
  for (int i = 0; i < 20; ++i) {
    values.push_back(2.0 + 0.05 * static_cast<double>(i));
    values.push_back(1.5 + 0.02 * static_cast<double>(i));
  }
  return FeatureBatch{
      .x = pluss::make_series(values, {40, 2}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

FeatureBatch embed_batch() {
  std::vector<double> values{};
  values.reserve(180);
  for (int i = 0; i < 60; ++i) {
    const double x0 = -2.0 + 4.0 * static_cast<double>(i) / 59.0;
    const double x1 = 0.8 * x0;
    const double x2 = -0.4 * x0 + 0.6 * x1;
    values.push_back(x0);
    values.push_back(x1);
    values.push_back(x2);
  }
  return FeatureBatch{
      .x = pluss::make_series(values, {60, 3}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

DataFrame cluster_frame() {
  const FeatureBatch batch = cluster_batch();
  const Series x = batch.x;
  const Series c0 = x.narrow_copy(1, 0, 1).reshape({40});
  const Series c1 = x.narrow_copy(1, 1, 1).reshape({40});
  return DataFrame({
      {"x0", Thread(c0)},
      {"x1", Thread(c1)},
  });
}

bool run_unsupervised_checks(std::ostream* err) {
  {
    ClusterFamilyOptions options{};
    options.clusters = 2;
    options.max_iterations = 200;
    options.restarts = 4;
    options.tolerance = 1e-5;
    TrainOptions train{};
    train.random.seed = 11;
    train.random.deterministic = true;

    const cluster::Result learned = cluster::model(models::kmeans(options)).run(cluster_batch(), train);
    const auto label_shape = learned.labels().sizes();
    const auto center_shape = learned.centers().sizes();
    if (!(learned.trained() && learned.labels().numel() == 40
          && center_shape.size() == 2 && center_shape[0] == 2 && center_shape[1] == 2
          && learned.cluster_count() == 2)) {
      if (err) *err << "cluster::model(...).run should return trained labels and fitted centers\n";
      return false;
    }
    if (!(label_shape.size() == 2 && label_shape[0] == 40 && label_shape[1] == 1)) {
      if (err) *err << "cluster labels should keep row-layout [samples, 1] shape\n";
      return false;
    }
    if (learned.board().ncases() != 2) {
      if (err) *err << "cluster::Result::board should summarize one row per cluster\n";
      return false;
    }

    Model reusable = models::kmeans(options);
    const cluster::Result via_model = cluster::model(reusable).run(cluster_frame(), train);
    if (!(via_model.trained() && !reusable.trained() && via_model.labels().numel() == 40)) {
      if (err) *err << "cluster::model(Model).run(DataFrame) should train a copied model result\n";
      return false;
    }
  }

  {
    DecomposeFamilyOptions options{};
    options.components = 2;
    options.max_iterations = 400;
    options.tolerance = 1e-6;
    TrainOptions train{};
    train.random.seed = 17;
    train.random.deterministic = true;

    const embed::Result learned = embed::model(models::pca(options)).run(embed_batch(), train);
    const auto coord_shape = learned.coordinates().sizes();
    const auto component_shape = learned.components().sizes();
    if (!(learned.trained()
          && coord_shape.size() == 2 && coord_shape[0] == 60 && coord_shape[1] == 2
          && component_shape.size() == 2 && component_shape[0] == 3 && component_shape[1] == 2
          && learned.variances().numel() == 2
          && learned.component_count() == 2)) {
      if (err) *err << "embed::model(...).run should return trained coordinates, components, and variances\n";
      return false;
    }
    if (learned.board().ncases() != 2) {
      if (err) *err << "embed::Result::board should summarize one row per component\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph unsupervised result families", "[lymph][unsupervised]") {
  REQUIRE(run_unsupervised_checks(&std::cerr));
}
#else
int main() {
  if (!run_unsupervised_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph unsupervised checks passed\n";
  return 0;
}
#endif
