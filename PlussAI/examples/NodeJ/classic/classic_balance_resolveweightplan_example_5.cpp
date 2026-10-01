#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 0.0, 1.0}, {4}, f64);
  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Balanced;

  const pluss::Series via_plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value().weights;
  const pluss::Series via_narrow = cls::resolve_training_weights(labels, pluss::series::undefined(), options).value();
  const double diff = (via_plan - via_narrow).abs().amax(0, false).item<double>();
  std::cout << "max abs diff = " << diff << "\n";

  return 0;
}
