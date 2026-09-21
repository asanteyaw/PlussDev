#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.9, 2.0, 2.1, 2.2}, {12}, f64).reshape({12, 1});
  const cls::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::ProbeSpecSampler sampler = [](std::int64_t, std::uint64_t seed) -> cls::StatusOr<cls::ModelSpec> {
    const double unit = static_cast<double>(seed % 1000) / 1000.0;
    cls::LinearFamilyOptions opts{};
    opts.objective = cls::LinearObjective::Logistic;
    opts.regularization_strength = 1e-3 + unit * (10.0 - 1e-3);
    return cls::StatusOr<cls::ModelSpec>(
        cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, opts});
  };

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.random.seed = 0;

  cls::RandomState random{};
  random.deterministic = true;
  random.seed = 7;

  const cls::TuneReport first = cls::probe(sampler, 4, batch, split, {}, {},
                                           cls::MetricKind::Accuracy,
                                           cls::TuneObjectivePolicy::Auto, random).value();
  const cls::TuneReport second = cls::probe(sampler, 4, batch, split, {}, {},
                                            cls::MetricKind::Accuracy,
                                            cls::TuneObjectivePolicy::Auto, random).value();

  bool identical = first.best_index == second.best_index;
  for (std::size_t i = 0; identical && i < first.trials.size(); ++i) {
    identical = identical && first.trials[i].objective == second.trials[i].objective
             && first.objective_rank[i] == second.objective_rank[i];
  }
  std::cout << "identical across both calls = " << std::boolalpha << identical << "\n";

  return 0;
}
