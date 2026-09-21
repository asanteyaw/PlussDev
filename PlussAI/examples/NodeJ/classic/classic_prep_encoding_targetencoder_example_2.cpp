#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // category 1 seen once with target 100; four other rows average target 0. prior = 20.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 2.0, 2.0, 2.0}, {5}, f64).reshape({5, 1});
  const pluss::Series target = pluss::make_series({100.0, 0.0, 0.0, 0.0, 0.0}, {5}, f64);

  auto fit_category_1 = [&](double smoothing) {
    cls::prep::TargetEncoderOptions options{};
    options.smoothing = smoothing;
    cls::prep::TargetEncoder encoder(options);
    encoder.learn_supervised(train, target);
    cls::FeatureBatch probe{};
    probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
    return encoder.map(probe).value().item<double>();
  };

  std::cout << "prior = 20\n";
  std::cout << "smoothing=0.1  -> " << fit_category_1(0.1) << "\n";
  std::cout << "smoothing=100  -> " << fit_category_1(100.0) << "\n";

  return 0;
}
