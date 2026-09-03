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

  // regularization is a Laplace term that keeps the log-odds finite for a
  // category that is all-positive or all-negative. Larger = more shrinkage
  // toward 0 (neutral).
  const pluss::Series region = pluss::make_series({0,0,0,1,1,1,2,2,2,2}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1,1,1,0,1}, {10}, f64);
  const DataFrame x({{"region", Thread(region)}});

  for (double r : {0.5, 20.0}) {
    lymph::options::WeightOfEvidence o{};
    o.regularization = r;
    lymph::models::Model model = lymph::models::logistic_regression();
    model.append(lymph::prep::stage::woe_encoder(o), lymph::prep::route::names({"region"}));
    model.learn_(x, y);
    std::cout << "regularization = " << r << " -> accuracy "
              << model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>() << "\n";
  }

  return 0;
}
