#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  // region carries no class information (each region is split 50/50); amount does.
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8.}, {8}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4, 0.8, 1.2}, {8}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 0, 1, 0, 1, 0, 1}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // PassThrough (the default) keeps amount and ratio, so the model can use the
  // feature that actually separates the classes.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));  // remainder defaults to PassThrough
  model.learn_(x, y);
  const auto acc = model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>();
  std::cout << "remainder = PassThrough -> region + amount + ratio, accuracy " << acc << "\n";
  std::cout << DataFrame({{"actual", Thread(y)}, {"predicted", Thread(model.infer(x))}});

  return 0;
}
