#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions bad_options{};
  bad_options.family.objective = cls::LinearObjective::LeastSquares;

  cls::online::OnlineLinearClassifier direct(bad_options);
  std::cout << "direct construction always succeeds (no ok() to check)\n";

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);
  const cls::Status learned = direct.learn(batch);
  std::cout << "direct.learn() ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "message = \"" << learned.message << "\"\n";

  const cls::StatusOr<cls::online::OnlineLinearClassifier> made =
      cls::online::make_online_linear_classifier(bad_options);
  std::cout << "make_online_linear_classifier ok = " << made.ok() << "\n";

  return 0;
}
