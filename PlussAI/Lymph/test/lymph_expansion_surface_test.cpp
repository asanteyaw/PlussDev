#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cmath>
#include <cstdint>
#include <functional>
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
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

FeatureBatch two_class_features() {
  std::vector<double> values{};
  values.reserve(32);
  for (int i = 0; i < 8; ++i) {
    values.push_back(-2.0 + 0.025 * static_cast<double>(i));
    values.push_back(-1.0 + 0.015 * static_cast<double>(i));
  }
  for (int i = 0; i < 8; ++i) {
    values.push_back(2.0 + 0.025 * static_cast<double>(i));
    values.push_back(1.0 + 0.015 * static_cast<double>(i));
  }
  return FeatureBatch{
      .x = pluss::make_series(values, {16, 2}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

LabeledBatch semi_batch() {
  std::vector<double> labels(16, -1.0);
  labels[0] = 0.0;
  labels[1] = 0.0;
  labels[8] = 1.0;
  labels[9] = 1.0;
  return LabeledBatch{
      .features = two_class_features(),
      .y = pluss::make_series(labels, {16}, f64()),
  };
}

FeatureBatch curved_features() {
  std::vector<double> values{};
  values.reserve(36);
  for (int i = 0; i < 12; ++i) {
    const double t = static_cast<double>(i + 1);
    values.push_back(std::exp(t / 8.0));
    values.push_back((i % 2 == 0) ? 0.0 : t);
    values.push_back(1.0);
  }
  return FeatureBatch{
      .x = pluss::make_series(values, {12, 3}, f64()),
      .layout = ObservationLayout::Rows,
  };
}

bool throws_pluss(const std::function<void()>& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    return true;
  }
  return false;
}

bool run_expansion_surface_checks(std::ostream* err) {
  {
    const shape::Result prof = shape::data(curved_features(), {"exp", "mixed", "constant"}).run();
    shape::ProfileOptions profile_options{};
    profile_options.skew_threshold = 0.2;
    profile_options.heavy_tail_threshold = 0.5;
    if (!(prof.sample_count() == 12
          && prof.feature_count() == 3
          && prof.board().ncases() == 3
          && prof.plan(profile_options).feature_count() >= 1)) {
      if (err) *err << "shape profile should summarize all features and propose at least one shape transform\n";
      return false;
    }
  }

  {
    ClusterFamilyOptions options{};
    options.clusters = 2;
    options.restarts = 1;
    TrainOptions train{};
    train.random.seed = 31;
    train.random.deterministic = true;
    const cluster::Result grouped = cluster::model(models::kmeans(options)).run(two_class_features(), train);
    if (!(grouped.trained()
          && grouped.labels().numel() == 16
          && grouped.cluster_count() == 2
          && grouped.board().ncases() == 2)) {
      if (err) *err << "cluster result should contain labels, centers, and a two-row board\n";
      return false;
    }
  }

  {
    DecomposeFamilyOptions options{};
    options.components = 1;
    const embed::Result mapped = embed::model(models::pca(options)).run(curved_features());
    if (!(mapped.trained()
          && mapped.coordinates().sizes().size() == 2
          && mapped.coordinates().sizes()[0] == 12
          && mapped.coordinates().sizes()[1] == 1
          && mapped.component_count() == 1)) {
      if (err) *err << "embed result should contain one learned component and mapped coordinates\n";
      return false;
    }
  }

  {
    prep::OutlierIsolatorOptions options{};
    options.estimators = 5;
    options.max_samples_auto = false;
    options.max_samples = 10;
    options.contamination_auto = false;
    options.contamination = 0.08;
    options.random.seed = 37;
    options.random.deterministic = true;
    const detect::Result found = detect::model(models::isolation_detector(options)).run(two_class_features());
    if (!(found.has_model()
          && found.labels().numel() == 16
          && found.flagged().numel() == 16
          && found.board().ncases() == 4)) {
      if (err) *err << "detect result should expose model-backed labels and summary board\n";
      return false;
    }
  }

  {
    semi::Options semi_options{};
    semi_options.select = semi::SelectKind::Confidence;
    semi_options.confidence = 0.0;
    semi_options.max_rounds = 1;
    const semi::Result learned = semi::model(models::gaussian_nb_classifier()).run(semi_batch(), semi_options);
    if (!(learned.model().trained()
          && learned.labels().numel() == 16
          && learned.labeled_count() == 16
          && learned.pseudo_labeled_count() == 12
          && learned.stop() == semi::StopReason::AllLabeled
          && learned.board().ncases() == 4)) {
      if (err) *err << "semi confidence run should pseudo-label every unlabeled row\n";
      return false;
    }
  }

  {
    semi::Options semi_options{};
    semi_options.select = semi::SelectKind::TopK;
    semi_options.top_k = 5;
    semi_options.max_rounds = 1;
    const semi::Result learned = semi::model(models::gaussian_nb_classifier()).run(semi_batch(), semi_options);
    if (!(learned.model().trained()
          && learned.labeled_count() == 9
          && learned.pseudo_labeled_count() == 5
          && learned.rounds() == 1
          && learned.stop() == semi::StopReason::MaxRounds)) {
      if (err) *err << "semi top-k run should add exactly top_k labels before max-round termination\n";
      return false;
    }
  }

  if (!throws_pluss([] {
        (void)cluster::model(models::pca()).run(curved_features());
      })) {
    if (err) *err << "cluster should reject non-cluster models\n";
    return false;
  }

  if (!throws_pluss([] {
        (void)detect::model(models::kmeans()).run(two_class_features());
      })) {
    if (err) *err << "detect should reject non-detector models\n";
    return false;
  }

  if (!throws_pluss([] {
        std::vector<double> labels(16, -1.0);
        (void)semi::model(models::knn_classifier()).run(LabeledBatch{
            .features = two_class_features(),
            .y = pluss::make_series(labels, {16}, f64()),
        });
      })) {
    if (err) *err << "semi should reject batches with no initially labeled rows\n";
    return false;
  }

  if (!throws_pluss([] {
        (void)semi::model(models::linear_regression()).run(semi_batch());
      })) {
    if (err) *err << "semi should reject non-classification models\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph first expansion surface is coherent", "[lymph][expansion]") {
  REQUIRE(run_expansion_surface_checks(&std::cerr));
}
#else
int main() {
  if (!run_expansion_surface_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph expansion surface checks passed\n";
  return 0;
}
#endif
