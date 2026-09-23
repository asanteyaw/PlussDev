#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {5.0, 0.0, 9.0, 0.0, 1.0, 0.1, 3.0, 0.2, 9.0, 0.2, 1.0, 0.1,
       2.0, 10.0, 5.0, 10.0, 8.0, 10.1, 2.0, 10.2, 3.0, 10.2, 8.0, 10.1},
      {24}, f64).reshape({6, 4});
  const cls::Series y = pluss::make_series({0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::FeaturePickOptions percent_form{};
  percent_form.fraction = 25.0;
  cls::FeaturePickOptions literal_form{};
  literal_form.fraction = 0.25;

  const cls::FeaturePickResult via_percent = cls::pick_features(batch, percent_form).value();
  const cls::FeaturePickResult via_literal = cls::pick_features(batch, literal_form).value();
  std::cout << "percent form feature_count() = " << via_percent.reduced.feature_count() << "\n";
  std::cout << "literal form feature_count() = " << via_literal.reduced.feature_count() << "\n";

  return 0;
}
