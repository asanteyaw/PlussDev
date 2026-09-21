#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::online::OnlineLinearOptions options{};
  options.family.objective = cls::LinearObjective::Logistic;
  cls::online::OnlineLinearClassifier model(options);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);

  const cls::Status before_learn = model.advance(batch);
  std::cout << "advance() before learn() ok = " << std::boolalpha << before_learn.ok() << "\n";
  std::cout << "message = \"" << before_learn.message << "\"\n";

  model.learn(batch);
  cls::LabeledBatch unseen_class{};
  unseen_class.features.x = pluss::make_series({3.0}, {1}, f64).reshape({1, 1});
  unseen_class.y = pluss::make_series({2.0}, {1}, f64);  // never seen at learn() time
  const cls::Status bad_label = model.advance(unseen_class);
  std::cout << "advance() with unseen label ok = " << bad_label.ok() << "\n";
  std::cout << "message = \"" << bad_label.message << "\"\n";

  return 0;
}
