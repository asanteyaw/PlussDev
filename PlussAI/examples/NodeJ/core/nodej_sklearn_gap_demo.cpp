#include <pluss/ml.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using namespace pluss::ai::nodej;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(std::vector<double>{
                                                 0.0, 1.0, 3.0,
                                                 0.0, 2.0, 3.1,
                                                 0.0, 3.0, 2.9,
                                                 0.0, 4.0, 3.0,
                                                 0.0, 5.0, 3.2,
                                                 0.0, 6.0, 2.8},
                                             {6, 3},
                                             f64);
  const pluss::Series y_class = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  const LabeledBatch class_batch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y_class,
  };

  const auto rank_or = rank_features(class_batch, FeatureRankOptions{
                                                      .mode = FeatureScoreMode::ClassF,
                                                      .absolute_score = true,
                                                  });
  if (!rank_or.ok()) {
    std::cerr << rank_or.status().message << "\n";
    return 1;
  }
  std::cout << "class scores: " << rank_or.value().scores << "\n";
  std::cout << "class rank: " << rank_or.value().ranked_feature_indices << "\n";

  const auto pick_or = pick_features(class_batch, FeaturePickOptions{
                                                      .mode = FeatureScoreMode::ClassF,
                                                      .top_k = 2,
                                                      .fraction = 0.0,
                                                      .absolute_score = true,
                                                  });
  if (!pick_or.ok()) {
    std::cerr << pick_or.status().message << "\n";
    return 1;
  }
  std::cout << "picked feature ids: " << pick_or.value().selected_feature_indices << "\n";
  std::cout << "picked shape: " << pick_or.value().reduced.sample_count()
            << "x" << pick_or.value().reduced.feature_count() << "\n";

  const pluss::Series y_reg = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64);
  const LabeledBatch reg_batch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y_reg,
  };
  const auto reg_pick_or = pick_features(reg_batch, FeaturePickOptions{
                                                        .mode = FeatureScoreMode::RegressionF,
                                                        .top_k = 10,
                                                        .fraction = 0.5,
                                                        .absolute_score = true,
                                                    });
  if (!reg_pick_or.ok()) {
    std::cerr << reg_pick_or.status().message << "\n";
    return 1;
  }
  std::cout << "regression picked ids: " << reg_pick_or.value().selected_feature_indices << "\n";

  const std::vector<ModelSpec> specs{
      ModelSpec{
          .task = TaskKind::Classification,
          .family = ModelFamily::Linear,
          .options = LinearFamilyOptions{
              .objective = LinearObjective::Logistic,
              .regularization = RegularizationKind::L2,
              .regularization_strength = 1e-3,
              .fit_intercept = true,
          },
      },
      ModelSpec{
          .task = TaskKind::Classification,
          .family = ModelFamily::Neighbor,
          .options = NeighborFamilyOptions{
              .k = 3,
              .query_mode = NeighborQueryMode::KNearest,
              .distance_weighted = true,
          },
      },
  };
  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
  split.folds = 3;
  split.shuffle = true;
  split.random.seed = 23;
  AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1};
  TrainOptions train{};
  train.max_iterations = 600;
  train.tolerance = 1e-7;

  const auto tune_or = sweep(specs, class_batch, split, train, assess, MetricKind::Accuracy, TuneObjectivePolicy::Maximize);
  if (!tune_or.ok()) {
    std::cerr << tune_or.status().message << "\n";
    return 1;
  }
  std::cout << "dense objective: " << tune_or.value().ledger.objective << "\n";
  std::cout << "dense ranks: " << tune_or.value().ledger.objective_rank << "\n";
  std::cout << "dense encoded params shape: "
            << tune_or.value().ledger.encoded_params.sizes()[0] << "x"
            << tune_or.value().ledger.encoded_params.sizes()[1] << "\n";

  const auto sparse_or = make_sparse_triplet(
      6,
      3,
      {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5},
      {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
      {-2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0});
  if (!sparse_or.ok()) {
    std::cerr << sparse_or.status().message << "\n";
    return 1;
  }
  const std::vector<LinearFamilyOptions> sparse_candidates{
      LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-3,
      },
      LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
      },
  };
  const auto sparse_tune_or = sweep_sparse_binary(sparse_candidates,
                                                  sparse_or.value(),
                                                  y_class,
                                                  split,
                                                  train,
                                                  AssessOptions{.metrics = {MetricKind::Accuracy}},
                                                  MetricKind::Accuracy,
                                                  SparseObjectivePolicy::Maximize);
  if (!sparse_tune_or.ok()) {
    std::cerr << sparse_tune_or.status().message << "\n";
    return 1;
  }
  std::cout << "sparse objective: " << sparse_tune_or.value().ledger.objective << "\n";
  std::cout << "sparse ranks: " << sparse_tune_or.value().ledger.objective_rank << "\n";
  std::cout << "sparse encoded params shape: "
            << sparse_tune_or.value().ledger.encoded_params.sizes()[0] << "x"
            << sparse_tune_or.value().ledger.encoded_params.sizes()[1] << "\n";

  return 0;
}
