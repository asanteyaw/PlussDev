// options example 1 -- the cross-cutting verb options:
// options::DFBatch names the columns, options::Train configures the fit,
// options::Assess picks which metrics assess(...) computes.

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame frame({
      {"income", Thread(pluss::make_series(
          {1.0, 1.2, 1.4, 1.1, 3.0, 3.2, 3.4, 3.1}, {8}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.5, 0.7, 0.6, 0.8, 2.5, 2.7, 2.6, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch batch{};
  batch.feature_names = {"income", "balance"};
  batch.label_feature = "default";

  lymph::options::Train train{};
  train.max_iterations = 250;
  train.class_weight = lymph::ClassWeightMode::Balanced;

  lymph::options::Assess assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1,
                    lymph::MetricKind::Mcc};

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(frame, batch, train);

  const lymph::metrics::MetricReport report = model.assess(frame, batch, assess);
  std::cout << "requested metric count = " << report.requested.size() << "\n";
  std::cout << "accuracy = " << report.accuracy.item<double>() << "\n";
  std::cout << "f1       = " << report.f1.item<double>() << "\n";
  std::cout << "mcc      = " << report.mcc.item<double>() << "\n";

  return 0;
}
