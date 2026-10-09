#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // smoothing controls how hard a rare category is pulled toward the global
  // mean. A larger value shrinks more. Two models, same data, different
  // smoothing -- both still fit this clean signal.
  const pluss::Series region = pluss::make_series({0,0,0,0,1,1,1,1,2,3}, {10}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1,1,0}, {10}, f64);
  const DataFrame x({{"region", Slate(region)}});

  for (double s : {1.0, 50.0}) {
    lymph::options::TargetEncoder o{};
    o.smoothing = s;
    lymph::models::Model model = lymph::models::logistic_regression();
    model.append(lymph::prep::stage::target_encoder(o), lymph::prep::route::names({"region"}));
    model.learn_(x, y);
    std::cout << "smoothing = " << s << " -> accuracy "
              << model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>() << "\n";
  }

  return 0;
}
