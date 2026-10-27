#include <pluss/ai/ml.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <unordered_map>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

std::vector<std::int64_t> as_i64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).host<std::int64_t>();
}

LabeledBatch make_xor_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -1.0, -1.0,
                                          -1.0,  1.0,
                                           1.0, -1.0,
                                           1.0,  1.0,
                                          -0.9, -0.9,
                                          -0.9,  0.9,
                                           0.9, -0.9,
                                           0.9,  0.9,
                                          -1.2, -1.1,
                                          -1.1,  1.2,
                                           1.1, -1.2,
                                           1.2,  1.1,
                                          -0.4, -0.7,
                                          -0.5,  0.8,
                                           0.6, -0.8,
                                           0.5,  0.7},
                                      {16, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0},
                                      {16},
                                      f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

ModelSpec linear_logistic_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };
}

ModelSpec rbf_kernel_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Rbf,
          .c = 40.0,
          .gamma = 2.0,
          .epsilon = 0.0,
      },
  };
}

ModelSpec linear_reg_spec(double lambda, bool fit_intercept = true) {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = lambda,
          .fit_intercept = fit_intercept,
      },
  };
}

bool run_nodej_cv_tune_checks(std::ostream* err = nullptr) {
  const LabeledBatch batch = make_xor_batch();

  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
  split.folds = 4;
  split.shuffle = true;
  split.random.seed = 73;
  split.random.deterministic = true;

  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;

  AssessOptions assess{};
  assess.metrics = {
      MetricKind::Accuracy,
      MetricKind::BalancedAccuracy,
      MetricKind::Precision,
      MetricKind::Recall,
      MetricKind::F1,
      MetricKind::RocAuc,
      MetricKind::LogLoss};

  {
    auto cv_or = cross_assess(linear_logistic_spec(), batch, split, train, assess);
    if (!cv_or.ok()) {
      if (err) *err << "cross_assess should succeed for linear logistic\n";
      return false;
    }
    const CrossAssessReport& cv = cv_or.value();
    if (cv.folds.size() != 4 || cv.metrics.size() != 7 || cv.mean_metrics.numel() != 7 || cv.std_metrics.numel() != 7) {
      if (err) *err << "cross_assess should return fold reports plus mean/std for requested metrics\n";
      return false;
    }
    if (!cv.mean_metrics.isfinite().all().item<bool>() || !cv.std_metrics.isfinite().all().item<bool>()) {
      if (err) *err << "cross_assess aggregate metrics should be finite\n";
      return false;
    }
    const auto mean = as_f64(cv.mean_metrics);
    if (mean.size() != 7 || mean[5] < 0.0 || mean[5] > 1.0) {
      if (err) *err << "ROC AUC should be present and bounded in [0,1]\n";
      return false;
    }
  }

  {
    SplitOptions repeated = split;
    repeated.plan = CVPlanKind::StratifiedCrossVal;
    repeated.folds = 4;
    repeated.repeats = 3;
    repeated.shuffle = true;
    repeated.random.seed = 91;
    repeated.random.deterministic = true;

    auto plan_or = stratified_crossval_plan(batch.y, repeated);
    if (!plan_or.ok()) {
      if (err) *err << "stratified_crossval_plan should succeed for balanced classification labels\n";
      return false;
    }
    const SplitPlan& repeated_plan = plan_or.value();
    if (repeated_plan.folds.size() != static_cast<std::size_t>(repeated.folds * repeated.repeats)) {
      if (err) *err << "stratified_crossval_plan should emit folds * repeats splits\n";
      return false;
    }
    std::vector<std::int64_t> test_counts(16, 0);
    const auto y = as_f64(batch.y);
    for (const auto& fold : repeated_plan.folds) {
      const auto test = as_i64(fold.test_indices);
      std::int64_t positives = 0;
      for (std::int64_t idx : test) {
        ++test_counts[static_cast<std::size_t>(idx)];
        if (y[static_cast<std::size_t>(idx)] > 0.5) ++positives;
      }
      if (test.size() != 4 || positives != 2) {
        if (err) *err << "stratified_crossval_plan should preserve class balance in each fold\n";
        return false;
      }
    }
    for (std::int64_t count : test_counts) {
      if (count != repeated.repeats) {
        if (err) *err << "stratified_crossval_plan should test each sample once per repeat\n";
        return false;
      }
    }

    auto repeated_cv_or = cross_assess(linear_logistic_spec(), batch, repeated, train, assess);
    if (!repeated_cv_or.ok()) {
      if (err) *err << "cross_assess StratifiedCrossVal should succeed\n";
      return false;
    }
    const CrossAssessReport& repeated_cv = repeated_cv_or.value();
    if (repeated_cv.folds.size() != repeated_plan.folds.size() || !repeated_cv.mean_metrics.isfinite().all().item<bool>()) {
      if (err) *err << "cross_assess StratifiedCrossVal should aggregate finite repeated fold metrics\n";
      return false;
    }
  }

  {
    SplitOptions group_split = split;
    group_split.plan = CVPlanKind::GroupCrossVal;
    group_split.folds = 4;
    group_split.shuffle = false;
    group_split.groups = pluss::make_series(std::vector<std::int64_t>{
                                                0, 0,
                                                1, 1,
                                                2, 2,
                                                3, 3,
                                                4, 4,
                                                5, 5,
                                                6, 6,
                                                7, 7},
                                            {16},
                                            Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));

    auto cv_or = cross_assess(linear_logistic_spec(), batch, group_split, train, assess);
    if (!cv_or.ok()) {
      if (err) *err << "cross_assess GroupCrossVal should succeed when groups are provided\n";
      return false;
    }
    const CrossAssessReport& cv = cv_or.value();
    if (cv.folds.size() != 4) {
      if (err) *err << "cross_assess GroupCrossVal should emit requested fold count\n";
      return false;
    }

    const auto groups = as_i64(group_split.groups);
    std::unordered_map<std::int64_t, std::int64_t> group_to_fold{};
    for (std::size_t fold_id = 0; fold_id < cv.folds.size(); ++fold_id) {
      const auto test = as_i64(cv.folds[fold_id].test_indices);
      for (std::int64_t idx : test) {
        const std::int64_t g = groups[static_cast<std::size_t>(idx)];
        const auto [it, inserted] = group_to_fold.emplace(g, static_cast<std::int64_t>(fold_id));
        if (!inserted && it->second != static_cast<std::int64_t>(fold_id)) {
          if (err) *err << "cross_assess GroupCrossVal should keep each group in exactly one test fold\n";
          return false;
        }
      }
    }
  }

  {
    const std::vector<ModelSpec> candidates{linear_logistic_spec(), rbf_kernel_spec()};
    AssessOptions tune_assess{};
    tune_assess.metrics = {MetricKind::Accuracy};
    auto sweep_or = sweep(candidates,
                          batch,
                          split,
                          train,
                          tune_assess,
                          MetricKind::Accuracy,
                          TuneObjectivePolicy::Auto);
    if (!sweep_or.ok()) {
      if (err) *err << "sweep should succeed for valid candidate list\n";
      return false;
    }

    const TuneReport& report = sweep_or.value();
    if (report.trials.size() != 2 || report.best_index < 0) {
      if (err) *err << "sweep should evaluate all candidates and choose a best trial\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()
        || report.objective_rank[static_cast<std::size_t>(report.best_index)] != 1) {
      if (err) *err << "sweep should populate objective ranks with best trial rank=1\n";
      return false;
    }
    if (report.ledger.objective.numel() != static_cast<std::int64_t>(report.trials.size())
        || report.ledger.objective_rank.numel() != static_cast<std::int64_t>(report.trials.size())
        || report.ledger.stage.numel() != static_cast<std::int64_t>(report.trials.size())
        || report.ledger.status_code.numel() != static_cast<std::int64_t>(report.trials.size())
        || report.ledger.encoded_params.sizes() != std::vector<std::int64_t>{static_cast<std::int64_t>(report.trials.size()), 40}) {
      if (err) *err << "sweep should populate tune ledger vectors and encoded params table\n";
      return false;
    }
    if (report.ledger.mean_test_metrics.sizes() != std::vector<std::int64_t>{static_cast<std::int64_t>(report.trials.size()), 1}
        || report.ledger.split_test_metrics.sizes().size() != 3) {
      if (err) *err << "sweep tune ledger should include mean/split test metric tables\n";
      return false;
    }
    if (report.ledger.encoded_param_feature_names.size() != 40) {
      if (err) *err << "sweep tune ledger should expose encoded feature names\n";
      return false;
    }
    if (!(report.best_index == 1)) {
      if (err) *err << "kernel candidate should win on XOR with accuracy objective\n";
      return false;
    }
  }

  {
    auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
      (void)seed;
      if (trial_index == 1) {
        return StatusOr<ModelSpec>(Status::invalid_argument("synthetic sampler failure"));
      }
      if ((trial_index % 2) == 0) {
        return StatusOr<ModelSpec>(linear_logistic_spec());
      }
      return StatusOr<ModelSpec>(rbf_kernel_spec());
    };

    AssessOptions tune_assess{};
    tune_assess.metrics = {MetricKind::Accuracy};
    auto probe_or = probe(sampler,
                          3,
                          batch,
                          split,
                          train,
                          tune_assess,
                          MetricKind::Accuracy,
                          TuneObjectivePolicy::Auto,
                          RandomState{.seed = 5, .deterministic = true});
    if (!probe_or.ok()) {
      if (err) *err << "probe should return report even if one sampled trial fails\n";
      return false;
    }

    const TuneReport& report = probe_or.value();
    if (report.trials.size() != 3) {
      if (err) *err << "probe should emit one trial per requested sample\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()) {
      if (err) *err << "probe should populate objective ranks for all trials\n";
      return false;
    }
    if (report.ledger.is_success.numel() != static_cast<std::int64_t>(report.trials.size())
        || report.ledger.split_test_metrics.sizes().size() != 3) {
      if (err) *err << "probe should populate tune ledger success/split tables\n";
      return false;
    }
    const auto success_flags = as_f64(report.ledger.is_success);
    if (success_flags.size() != 3 || success_flags[1] != 0.0) {
      if (err) *err << "probe ledger should mark failed trial as unsuccessful\n";
      return false;
    }
    const auto status_codes = as_i64(report.ledger.status_code);
    if (status_codes.size() != 3 || status_codes[1] != static_cast<std::int64_t>(StatusCode::InvalidArgument)) {
      if (err) *err << "probe ledger should keep failure status code per trial\n";
      return false;
    }
    if (report.trials[1].status.code != StatusCode::InvalidArgument) {
      if (err) *err << "probe should preserve sampler failure status in the corresponding trial\n";
      return false;
    }
  }

  {
    const std::vector<ModelSpec> candidates{linear_logistic_spec(), rbf_kernel_spec()};
    AssessOptions tune_assess{};
    tune_assess.metrics = {MetricKind::Accuracy};

    TuneOptions tune_options{};
    tune_options.search = SearchKind::Grid;
    tune_options.max_parallel = 2;
    tune_options.split = split;

    auto tune_or = tune(candidates,
                        batch,
                        tune_options,
                        train,
                        tune_assess,
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune grid dispatcher should succeed for candidate list\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() != 2 || report.best_index < 0) {
      if (err) *err << "tune grid should evaluate all candidates and choose a best trial\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()
        || report.objective_rank[static_cast<std::size_t>(report.best_index)] != 1) {
      if (err) *err << "tune grid should return rank metadata\n";
      return false;
    }
  }

  {
    TuneOptions tune_options{};
    tune_options.search = SearchKind::Random;
    tune_options.trials = 5;
    tune_options.max_parallel = 2;
    tune_options.random.seed = 91;
    tune_options.random.deterministic = true;
    tune_options.split = split;

    auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
      return StatusOr<ModelSpec>(((seed + static_cast<std::uint64_t>(trial_index)) % 2) == 0
                                     ? linear_logistic_spec()
                                     : rbf_kernel_spec());
    };

    auto tune_or = tune(sampler,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune random dispatcher should succeed for sampler path\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() != 5 || report.best_index < 0) {
      if (err) *err << "tune random should emit requested trial count and a best index\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()) {
      if (err) *err << "tune random should return rank metadata\n";
      return false;
    }
  }

  {
    const std::vector<ModelSpec> candidates{
        linear_logistic_spec(),
        linear_logistic_spec(),
        rbf_kernel_spec(),
        rbf_kernel_spec(),
    };
    TuneOptions tune_options{};
    tune_options.search = SearchKind::Random;
    tune_options.trials = 3;
    tune_options.max_parallel = 2;
    tune_options.random.seed = 13;
    tune_options.random.deterministic = true;
    tune_options.split = split;

    auto tune_or = tune(candidates,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune random dispatcher should support candidate-list sampling\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() != 3 || report.best_index < 0) {
      if (err) *err << "candidate-list random tuning should evaluate sampled trial count\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()) {
      if (err) *err << "candidate-list random tuning should return rank metadata\n";
      return false;
    }
  }

  {
    const std::vector<ModelSpec> candidates{
        linear_logistic_spec(),
        linear_logistic_spec(),
        linear_logistic_spec(),
        rbf_kernel_spec(),
        rbf_kernel_spec(),
    };
    TuneOptions tune_options{};
    tune_options.search = SearchKind::Bayesian;
    tune_options.trials = 4;
    tune_options.random.seed = 123;
    tune_options.random.deterministic = true;
    tune_options.split = split;

    auto tune_or = tune(candidates,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune bayesian should succeed for candidate-list path\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() != 4 || report.best_index < 0) {
      if (err) *err << "tune bayesian should respect trial budget and produce best index\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()
        || report.objective_rank[static_cast<std::size_t>(report.best_index)] != 1) {
      if (err) *err << "tune bayesian should return rank metadata\n";
      return false;
    }
    std::int64_t max_stage = 0;
    for (const auto& trial : report.trials) {
      max_stage = std::max<std::int64_t>(max_stage, trial.stage);
    }
    if (max_stage < 2) {
      if (err) *err << "tune bayesian should run sequential staged selection\n";
      return false;
    }
  }

  {
    const std::vector<ModelSpec> candidates{
        linear_logistic_spec(),
        linear_logistic_spec(),
        linear_logistic_spec(),
        rbf_kernel_spec(),
    };
    TuneOptions tune_options{};
    tune_options.search = SearchKind::SuccessiveHalving;
    tune_options.max_parallel = 2;
    tune_options.split = split;

    auto tune_or = tune(candidates,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune successive-halving dispatcher should succeed for candidate list\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() <= candidates.size() || report.best_index < 0) {
      if (err) *err << "tune successive-halving should run multi-round elimination\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()
        || report.objective_rank[static_cast<std::size_t>(report.best_index)] != 1) {
      if (err) *err << "tune successive-halving should return rank metadata\n";
      return false;
    }
    if (report.ledger.stage.numel() != static_cast<std::int64_t>(report.trials.size())) {
      if (err) *err << "tune successive-halving should include stage column in tune ledger\n";
      return false;
    }
    std::int64_t max_stage = 0;
    for (const auto& trial : report.trials) {
      max_stage = std::max<std::int64_t>(max_stage, trial.stage);
    }
    if (max_stage <= 0) {
      if (err) *err << "tune successive-halving should annotate multi-stage trial progression\n";
      return false;
    }
    if (report.trials[static_cast<std::size_t>(report.best_index)].spec.family != ModelFamily::Kernel) {
      if (err) *err << "tune successive-halving should retain best kernel trial on XOR\n";
      return false;
    }
  }

  {
    TuneOptions tune_options{};
    tune_options.search = SearchKind::SuccessiveHalving;
    tune_options.trials = 5;
    tune_options.max_parallel = 2;
    tune_options.random.seed = 19;
    tune_options.random.deterministic = true;
    tune_options.split = split;

    auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
      return StatusOr<ModelSpec>(((seed + static_cast<std::uint64_t>(trial_index)) % 3) == 0
                                     ? rbf_kernel_spec()
                                     : linear_logistic_spec());
    };

    auto tune_or = tune(sampler,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune successive-halving dispatcher should support sampler path\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() <= static_cast<std::size_t>(tune_options.trials) || report.best_index < 0) {
      if (err) *err << "sampler successive-halving should evaluate across multiple rounds\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()) {
      if (err) *err << "sampler successive-halving should return rank metadata\n";
      return false;
    }
  }

  {
    TuneOptions tune_options{};
    tune_options.search = SearchKind::Bayesian;
    tune_options.trials = 4;
    tune_options.random.seed = 77;
    tune_options.random.deterministic = true;
    tune_options.split = split;

    auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
      return StatusOr<ModelSpec>(((seed + static_cast<std::uint64_t>(trial_index)) % 5) <= 1
                                     ? rbf_kernel_spec()
                                     : linear_logistic_spec());
    };

    auto tune_or = tune(sampler,
                        batch,
                        tune_options,
                        train,
                        AssessOptions{.metrics = {MetricKind::Accuracy}},
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto);
    if (!tune_or.ok()) {
      if (err) *err << "tune bayesian should succeed for sampler path\n";
      return false;
    }
    const TuneReport& report = tune_or.value();
    if (report.trials.size() != 4 || report.best_index < 0) {
      if (err) *err << "sampler bayesian should respect trial budget and produce best index\n";
      return false;
    }
    if (report.objective_rank.size() != report.trials.size()) {
      if (err) *err << "sampler bayesian should return rank metadata\n";
      return false;
    }
    if (report.ledger.objective_rank.numel() != static_cast<std::int64_t>(report.trials.size())) {
      if (err) *err << "sampler bayesian should populate ledger objective ranks\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0,
                                            -2.0,
                                            -1.0,
                                             1.0,
                                             2.0,
                                             3.0},
                                        {6, 1},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch easy{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto free_or = make_classifier(linear_logistic_spec());
    auto bounded_or = make_classifier(linear_logistic_spec());
    if (!free_or.ok() || !bounded_or.ok()) {
      if (err) *err << "make_classifier should succeed for linear logistic\n";
      return false;
    }

    TrainOptions free_train{};
    free_train.max_iterations = 1800;
    free_train.tolerance = 1e-8;

    TrainOptions bounded = free_train;
    bounded.parameter_lower_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
    bounded.parameter_upper_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());

    auto free = free_or.move_value();
    auto bounded_model = bounded_or.move_value();
    if (!free->learn(easy, free_train).ok() || !bounded_model->learn(easy, bounded).ok()) {
      if (err) *err << "linear learn should succeed with and without parameter bounds\n";
      return false;
    }

    auto free_acc_or = free->assess(easy, AssessOptions{.metrics = {MetricKind::Accuracy}});
    auto bounded_acc_or = bounded_model->assess(easy, AssessOptions{.metrics = {MetricKind::Accuracy}});
    auto bounded_scores_or = bounded_model->infer_scores(easy.features);
    if (!free_acc_or.ok() || !bounded_acc_or.ok() || !bounded_scores_or.ok()) {
      if (err) *err << "assess/infer_scores should succeed after learn\n";
      return false;
    }

    const double free_acc = as_f64(free_acc_or.value())[0];
    const double bounded_acc = as_f64(bounded_acc_or.value())[0];
    const double bounded_max_abs_score = bounded_scores_or.value().abs().max().item<double>();

    if (free_acc < 0.99) {
      if (err) *err << "unconstrained logistic should fit separable data\n";
      return false;
    }
    if (!(bounded_acc >= 0.45 && bounded_acc <= 0.55)) {
      if (err) *err << "zero-bounded logistic should collapse near chance-level on balanced data\n";
      return false;
    }
    if (!std::isfinite(bounded_max_abs_score) || bounded_max_abs_score > 1e-8) {
      if (err) *err << "zero-bounded logistic should produce near-zero scores\n";
      return false;
    }
  }

  {
    const std::int64_t n = 80;
    std::vector<double> xv{};
    std::vector<double> yv{};
    xv.reserve(static_cast<std::size_t>(n));
    yv.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      const double x = -2.0 + 4.0 * static_cast<double>(i) / static_cast<double>(n - 1);
      const double y = 2.0 * x + 1.0 + ((i % 9) == 0 ? 0.35 : 0.0);
      xv.push_back(x);
      yv.push_back(y);
    }

    const LabeledBatch reg_batch{
        .features = FeatureBatch{.x = pluss::make_series(std::move(xv), {n, 1}, f64()), .layout = ObservationLayout::Rows},
        .y = pluss::make_series(std::move(yv), {n}, f64()),
    };

    AssessOptions reg_assess{};
    reg_assess.metrics = {MetricKind::QuantileLoss, MetricKind::Mse};
    reg_assess.regression_quantile = 0.8;

    const std::vector<ModelSpec> candidates{
        linear_reg_spec(1e-9, true),
        linear_reg_spec(200.0, true),
    };

    auto sweep_or = sweep(candidates,
                          reg_batch,
                          split,
                          TrainOptions{},
                          reg_assess,
                          MetricKind::QuantileLoss,
                          TuneObjectivePolicy::Auto);
    if (!sweep_or.ok()) {
      if (err) *err << "sweep should support QuantileLoss objective\n";
      return false;
    }

    const TuneReport& report = sweep_or.value();
    if (report.trials.size() != 2 || report.best_index < 0) {
      if (err) *err << "QuantileLoss sweep should evaluate and pick a best trial\n";
      return false;
    }
    double min_objective = std::numeric_limits<double>::infinity();
    std::int64_t min_index = -1;
    for (std::size_t i = 0; i < report.trials.size(); ++i) {
      if (!report.trials[i].status.ok()) continue;
      const double objective = report.trials[i].objective;
      if (!std::isfinite(objective)) continue;
      if (objective < min_objective) {
        min_objective = objective;
        min_index = static_cast<std::int64_t>(i);
      }
    }
    if (min_index < 0 || report.best_index != min_index) {
      if (err) *err << "Auto objective policy should minimize QuantileLoss\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0}, {5}, f64());
    const LabeledBatch batch_reg{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto reg_or = make_regressor(linear_reg_spec(0.0, false));
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for linear regression\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch_reg).ok()) {
      if (err) *err << "linear regressor should learn on simple data\n";
      return false;
    }

    constexpr double q = 0.8;
    auto pred_or = reg->infer(batch_reg.features);
    auto qloss_or = reg->assess(batch_reg, AssessOptions{.metrics = {MetricKind::QuantileLoss}, .regression_quantile = q});
    if (!pred_or.ok() || !qloss_or.ok()) {
      if (err) *err << "infer and quantile assess should succeed\n";
      return false;
    }

    const auto y_true = as_f64(y);
    const auto y_pred = as_f64(pred_or.value());
    double manual = 0.0;
    for (std::size_t i = 0; i < y_true.size(); ++i) {
      const double r = y_true[i] - y_pred[i];
      manual += r * (q - (r < 0.0 ? 1.0 : 0.0));
    }
    manual /= static_cast<double>(y_true.size());

    const double got = as_f64(qloss_or.value())[0];
    if (!std::isfinite(got) || std::abs(got - manual) > 1e-10) {
      if (err) *err << "QuantileLoss should match pinball-loss definition\n";
      return false;
    }

    auto bad_q_or = reg->assess(batch_reg, AssessOptions{.metrics = {MetricKind::QuantileLoss}, .regression_quantile = 1.0});
    if (bad_q_or.ok() || bad_q_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "invalid quantile should return InvalidArgument\n";
      return false;
    }
  }

  {
    namespace lymph = pluss::ai::lymph;
    const Series x_series = pluss::make_series(
        std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0},
        {6},
        f64());
    const Series y = pluss::make_series(
        std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0},
        {6},
        f64());
    const pluss::table::DataFrame x({
        {"x", pluss::table::Slate(x_series)},
    });

    std::vector<lymph::selection::Candidate> candidates;
    candidates.emplace_back(
        "linear",
        lymph::models::linear_regression(LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::None,
            .fit_intercept = true,
        }));
    candidates.emplace_back(
        "ridge",
        lymph::models::linear_regression(LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 10.0,
            .fit_intercept = true,
        }));

    lymph::selection::CrossValOptions cv{};
    cv.plan = CVPlanKind::CrossVal;
    cv.folds = 3;
    cv.shuffle = false;
    cv.max_parallel = 2;
    lymph::AssessOptions assess{};
    assess.metrics = {MetricKind::Rmse};

    auto tuned = lymph::selection::compare(
        std::move(candidates),
        x,
        y,
        cv,
        TrainOptions{},
        assess,
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize,
        lymph::selection::FitOptions{.fit_top = 5});
    if (!tuned.best().trained()) {
      if (err) *err << "lymph selection::compare should return a fitted best workflow\n";
      return false;
    }
    if (tuned.fitted.size() != tuned.report.trials.size() || tuned.fitted.empty() || !tuned.fitted[0].trained()) {
      if (err) *err << "lymph selection::compare should fit requested top-k workflows in ranked order\n";
      return false;
    }
    auto top2 = tuned.top(2);
    if (top2.size() != 2 || !top2[0].second.trained() || !top2[1].second.trained()) {
      if (err) *err << "lymph selection::CompareResult::top should expose fitted ranked workflows\n";
      return false;
    }
    if (tuned.report.objective_rank.size() != tuned.report.trials.size()
        || tuned.report.ranked_indices.size() != tuned.report.trials.size()
        || tuned.report.objective_rank[static_cast<std::size_t>(tuned.report.best_index)] != 1) {
      if (err) *err << "lymph selection::compare should return ranked trial metadata\n";
      return false;
    }
    const Series pred = tuned.best().infer(x);
    if (!pred.defined() || pred.numel() != y.numel()) {
      if (err) *err << "fitted lymph best workflow should infer without a second learn_ call\n";
      return false;
    }
  }

  {
    // .board(): results collapse to a DataFrame instead of a hand-rolled
    // std::vector<...> + push_back loop (CompareReport::board() and
    // CrossValidationReport::board()).
    namespace lymph = pluss::ai::lymph;
    const Series x_series = pluss::make_series(
        std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, {6}, f64());
    const Series y = pluss::make_series(
        std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0}, {6}, f64());
    const pluss::table::DataFrame x({{"x", pluss::table::Slate(x_series)}});

    std::vector<lymph::selection::Candidate> candidates;
    candidates.emplace_back(
        "linear",
        lymph::models::linear_regression(LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::None,
            .fit_intercept = true,
        }));
    candidates.emplace_back(
        "ridge",
        lymph::models::linear_regression(LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 10.0,
            .fit_intercept = true,
        }));

    lymph::selection::CrossValOptions cv{};
    cv.plan = CVPlanKind::CrossVal;
    cv.folds = 3;
    cv.shuffle = false;
    cv.max_parallel = 2;
    lymph::AssessOptions assess{};
    assess.metrics = {MetricKind::Rmse};

    auto tuned = lymph::selection::compare(
        std::move(candidates), x, y, cv, TrainOptions{}, assess, MetricKind::Rmse, TuneObjectivePolicy::Minimize);

    const auto board = tuned.report.board();
    if (board.ncases() != static_cast<std::int64_t>(tuned.report.trials.size())) {
      if (err) *err << "CompareReport::board() should have one row per trial\n";
      return false;
    }
    if (!board.get("name").has_value() || !board.get("objective").has_value() ||
        !board.get("rank").has_value() || !board.get("rmse").has_value()) {
      if (err) *err << "CompareReport::board() should have name/objective/rank/<metric> columns\n";
      return false;
    }
    const auto objective_col = as_f64(board.features("objective").as_series().reshape({board.ncases()}));
    const auto rmse_col = as_f64(board.features("rmse").as_series().reshape({board.ncases()}));
    for (std::size_t i = 0; i < tuned.report.trials.size(); ++i) {
      if (std::abs(objective_col[i] - tuned.report.trials[i].objective) > 1e-10 ||
          std::abs(rmse_col[i] - tuned.report.trials[i].objective) > 1e-10) {
        if (err) *err << "CompareReport::board() objective/rmse columns should match trial objectives\n";
        return false;
      }
    }

    const auto cv_board = tuned.report.trials.front().cv.board();
    if (cv_board.ncases() != 1) {
      if (err) *err << "CrossValidationReport::board() should have one row per requested metric\n";
      return false;
    }
    const auto cv_mean = as_f64(cv_board.features("mean").as_series().reshape({cv_board.ncases()}));
    if (std::abs(cv_mean[0] - tuned.report.trials.front().objective) > 1e-10) {
      if (err) *err << "CrossValidationReport::board() mean should match the trial's cross-validated metric\n";
      return false;
    }
  }

  {
    // search::grid/search::random: genuine hyperparameter search over a
    // GridAxis space, with the trial factory building full Models instead of
    // bare ModelSpecs. Distinct from selection::compare above, which only
    // ranks candidates already fully built by the caller.
    namespace lymph = pluss::ai::lymph;

    // grid_size/grid_point_at/grid_points: a tiny 2x3 space, checked for
    // exact point count and full coverage (every (a,b) pair appears once).
    const std::vector<lymph::selection::GridAxis> tiny_axes = {
        {.name = "a", .values = {10.0, 20.0}},
        {.name = "b", .values = {1.0, 2.0, 3.0}},
    };
    if (lymph::selection::grid_size(tiny_axes) != 6) {
      if (err) *err << "selection::grid_size should be the product of axis sizes\n";
      return false;
    }
    const auto tiny_points = lymph::selection::grid_points(tiny_axes);
    if (tiny_points.size() != 6) {
      if (err) *err << "selection::grid_points should materialize grid_size() points\n";
      return false;
    }
    std::set<std::pair<double, double>> seen_pairs;
    for (const auto& point : tiny_points) {
      seen_pairs.emplace(point.at("a"), point.at("b"));
    }
    if (seen_pairs.size() != 6) {
      if (err) *err << "selection::grid_points should cover every axis combination exactly once\n";
      return false;
    }

    // Real search: on noiseless y = 2x + 1 data, the least-biased
    // (smallest regularization_strength) point should win.
    const Series x_series = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {6}, f64());
    const Series y_series = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0}, {6}, f64());
    const pluss::table::DataFrame tune_x({{"x", pluss::table::Slate(x_series)}});

    const std::vector<lymph::selection::GridAxis> hp_axes = {
        {.name = "regularization_strength", .values = {0.0001, 0.01, 1.0, 10.0}},
    };
    const auto make_linear_model = [](const lymph::selection::GridPoint& point) {
      return lymph::models::linear_regression(LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = point.at("regularization_strength"),
          .fit_intercept = true,
      });
    };

    lymph::selection::CrossValOptions hp_cv{};
    hp_cv.plan = CVPlanKind::CrossVal;
    hp_cv.folds = 3;
    hp_cv.shuffle = false;

    const auto exhaustive = lymph::selection::grid(hp_axes, hp_cv).run(
        make_linear_model,
        tune_x,
        y_series,
        TrainOptions{},
        AssessOptions{.metrics = {MetricKind::Rmse}},
        MetricKind::Rmse,
        TuneObjectivePolicy::Minimize);
    if (exhaustive.report.trials.size() != 4) {
      if (err) *err << "search::grid(...).run(...) should evaluate exactly grid_size(axes) trials\n";
      return false;
    }
    const auto& best_options = std::get<LinearFamilyOptions>(exhaustive.best().model_spec().value().options);
    if (best_options.regularization_strength > 0.001) {
      if (err) *err << "search::grid(...).run(...) should prefer the least-biased fit on noiseless linear data\n";
      return false;
    }

    const auto sampled = lymph::selection::random(
                             hp_axes,
                             2,
                             RandomState{.seed = 7, .deterministic = true},
                             hp_cv)
                             .run(make_linear_model,
                                  tune_x,
                                  y_series,
                                  TrainOptions{},
                                  AssessOptions{.metrics = {MetricKind::Rmse}},
                                  MetricKind::Rmse,
                                  TuneObjectivePolicy::Minimize);
    if (sampled.report.trials.size() != 2) {
      if (err) *err << "search::random(...).run(...) should evaluate exactly the requested trial count\n";
      return false;
    }
    if (sampled.report.trials[0].name == sampled.report.trials[1].name) {
      if (err) *err << "search::random(...).run(...) should draw distinct grid points without replacement\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej cross-assess, tuning, and constrained linear optimization") {
  REQUIRE(run_nodej_cv_tune_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_cv_tune_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_cv_tune_test passed\n";
  return 0;
}
#endif
