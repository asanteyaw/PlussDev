#include <pluss/ml.h>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
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

bool require_status(const Status& st, const std::string& label) {
  if (st.ok()) return true;
  std::cerr << label << " failed: " << st.message << "\n";
  return false;
}

void print_metric(const std::string& label, const Series& m) {
  const auto v = m.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << label << ":";
  for (double x : v) std::cout << " " << x;
  std::cout << "\n";
}

void run_classifier_family(const std::string& name, const ModelSpec& spec, const LabeledBatch& train) {
  auto model_or = make_classifier(spec);
  if (!require_ok(model_or, name + " make_classifier")) return;
  std::unique_ptr<ClassifierBase> model = model_or.move_value();
  if (!require_status(model->learn(train), name + " learn")) return;

  auto pred_or = model->infer(train.features);
  if (!require_ok(pred_or, name + " infer")) return;
  auto metric_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
  if (!require_ok(metric_or, name + " assess")) return;
  print_metric("  " + name + " (acc, logloss)", metric_or.value());
}

void run_regressor_family(const std::string& name, const ModelSpec& spec, const LabeledBatch& train) {
  auto model_or = make_regressor(spec);
  if (!require_ok(model_or, name + " make_regressor")) return;
  std::unique_ptr<RegressorBase> model = model_or.move_value();
  if (!require_status(model->learn(train), name + " learn")) return;

  auto pred_or = model->infer(train.features);
  if (!require_ok(pred_or, name + " infer")) return;
  auto metric_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!require_ok(metric_or, name + " assess")) return;
  print_metric("  " + name + " (mse, r2)", metric_or.value());
}

} // namespace

int main() {
  std::cout << "NodeJ model families demo\n";

  const Series x_cls = pluss::make_series(std::vector<double>{
                                              -2.0, -1.0,
                                              -1.5, -1.0,
                                              -2.0, -2.0,
                                               1.0,  1.0,
                                               2.0,  1.0,
                                               1.0,  2.0},
                                          {6, 2},
                                          f64());
  const Series y_cls = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
  const LabeledBatch cls_train{
      .features = FeatureBatch{.x = x_cls, .layout = ObservationLayout::Rows},
      .y = y_cls,
  };

  std::cout << "Classification families:\n";
  run_classifier_family("linear", ModelSpec{
                                      .task = TaskKind::Classification,
                                      .family = ModelFamily::Linear,
                                      .options = LinearFamilyOptions{
                                          .objective = LinearObjective::Logistic,
                                          .regularization = RegularizationKind::L2,
                                          .regularization_strength = 1e-6,
                                          .fit_intercept = true,
                                      },
                                  },
                        cls_train);
  run_classifier_family("linear_hinge", ModelSpec{
                                            .task = TaskKind::Classification,
                                            .family = ModelFamily::Linear,
                                            .options = LinearFamilyOptions{
                                                .objective = LinearObjective::Hinge,
                                                .regularization = RegularizationKind::L2,
                                                .regularization_strength = 1e-6,
                                                .fit_intercept = true,
                                            },
                                        },
                        cls_train);
  run_classifier_family("tree", ModelSpec{
                                    .task = TaskKind::Classification,
                                    .family = ModelFamily::Tree,
                                    .options = TreeFamilyOptions{},
                                },
                        cls_train);
  run_classifier_family("ensemble", ModelSpec{
                                        .task = TaskKind::Classification,
                                        .family = ModelFamily::Ensemble,
                                        .options = EnsembleFamilyOptions{
                                            .method = EnsembleMethod::RandomForest,
                                            .learners = 21,
                                            .subsample = 0.9,
                                        },
                                    },
                        cls_train);
  run_classifier_family("neighbor", ModelSpec{
                                        .task = TaskKind::Classification,
                                        .family = ModelFamily::Neighbor,
                                        .options = NeighborFamilyOptions{
                                            .k = 3,
                                            .distance_weighted = true,
                                            .standardize = true,
                                        },
                                    },
                        cls_train);
  run_classifier_family("kernel", ModelSpec{
                                      .task = TaskKind::Classification,
                                      .family = ModelFamily::Kernel,
                                      .options = KernelFamilyOptions{
                                          .kernel = KernelKind::Rbf,
                                          .c = 30.0,
                                          .gamma = 1.0,
                                          .epsilon = 0.0,
                                      },
                                  },
                        cls_train);
  run_classifier_family("bayes", ModelSpec{
                                     .task = TaskKind::Classification,
                                     .family = ModelFamily::Bayes,
                                     .options = BayesFamilyOptions{
                                         .use_gaussian = true,
                                         .smoothing = 1e-9,
                                     },
                                 },
                        cls_train);

  const Series x_reg = pluss::make_series(std::vector<double>{
                                              -2.0, -1.0,
                                              -1.0, -0.5,
                                               0.0,  0.0,
                                               1.0,  0.5,
                                               2.0,  1.0,
                                               3.0,  1.5},
                                          {6, 2},
                                          f64());
  const Series y_reg = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
  const LabeledBatch reg_train{
      .features = FeatureBatch{.x = x_reg, .layout = ObservationLayout::Rows},
      .y = y_reg,
  };

  std::cout << "Regression families:\n";
  run_regressor_family("linear", ModelSpec{
                                     .task = TaskKind::Regression,
                                     .family = ModelFamily::Linear,
                                     .options = LinearFamilyOptions{
                                         .objective = LinearObjective::LeastSquares,
                                         .regularization = RegularizationKind::L2,
                                         .regularization_strength = 1e-8,
                                         .fit_intercept = true,
                                     },
                                 },
                       reg_train);
  run_regressor_family("tree", ModelSpec{
                                   .task = TaskKind::Regression,
                                   .family = ModelFamily::Tree,
                                   .options = TreeFamilyOptions{
                                       .criterion = TreeCriterion::Mse,
                                   },
                               },
                       reg_train);
  run_regressor_family("ensemble", ModelSpec{
                                       .task = TaskKind::Regression,
                                       .family = ModelFamily::Ensemble,
                                       .options = EnsembleFamilyOptions{
                                           .method = EnsembleMethod::RandomForest,
                                           .learners = 31,
                                           .subsample = 0.9,
                                       },
                                   },
                       reg_train);
  run_regressor_family("neighbor", ModelSpec{
                                       .task = TaskKind::Regression,
                                       .family = ModelFamily::Neighbor,
                                       .options = NeighborFamilyOptions{
                                           .k = 3,
                                           .distance_weighted = true,
                                           .standardize = true,
                                       },
                                   },
                       reg_train);
  run_regressor_family("kernel", ModelSpec{
                                     .task = TaskKind::Regression,
                                     .family = ModelFamily::Kernel,
                                     .options = KernelFamilyOptions{
                                         .kernel = KernelKind::Rbf,
                                         .c = 50.0,
                                         .gamma = 0.7,
                                         .epsilon = 0.0,
                                     },
                                 },
                       reg_train);
  run_regressor_family("bayes", ModelSpec{
                                    .task = TaskKind::Regression,
                                    .family = ModelFamily::Bayes,
                                    .options = BayesFamilyOptions{
                                        .use_gaussian = true,
                                        .smoothing = 1e-6,
                                    },
                                },
                       reg_train);

  std::cout << "NodeJ model families demo completed.\n";
  return 0;
}
