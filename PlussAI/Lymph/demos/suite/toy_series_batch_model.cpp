#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;

  const Series x = pluss::series::linspace(-2.0, 2.0, 70, ScalarType::Float64);
  const Series x2 = x.square();
  const Series matrix = x.unite({x2}, 0).reshape({70, 2});
  const Series y = x.mul(1.5).add(x2.mul(0.25));

  lymph::LabeledBatch batch{
      .features = lymph::FeatureBatch{.x = matrix, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };

  lymph::Model model = lymph::model::linear_regression({.regularization_strength = 0.001});
  model.learn_(batch);
  const lymph::MetricReport report = model.assess(batch);

  std::cout << "series-batch r2: " << report.r2.item<double>() << "\n";
  return 0;
}
