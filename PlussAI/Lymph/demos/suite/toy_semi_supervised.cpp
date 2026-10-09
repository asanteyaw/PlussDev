#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::Config;
  using pluss::ScalarType;

  const Config f64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  std::vector<double> x{};
  x.reserve(80);
  std::vector<double> labels(40, -1.0);
  for (int i = 0; i < 20; ++i) {
    x.push_back(-2.0 + static_cast<double>(i) * 0.03);
    x.push_back(-1.0 + static_cast<double>(i) * 0.02);
  }
  for (int i = 0; i < 20; ++i) {
    x.push_back(2.0 + static_cast<double>(i) * 0.03);
    x.push_back(1.0 + static_cast<double>(i) * 0.02);
  }
  labels[0] = 0.0;
  labels[1] = 0.0;
  labels[20] = 1.0;
  labels[21] = 1.0;

  lymph::models::semi::Options options{};
  options.select = lymph::models::semi::SelectKind::TopK;
  options.top_k = 8;
  options.max_rounds = 2;

  const lymph::models::semi::Result result =
      lymph::models::semi::model(lymph::models::gaussian_nb_classifier())
          .run(lymph::LabeledBatch{
                   .features = lymph::FeatureBatch{
                       .x = pluss::make_series(x, {40, 2}, f64),
                       .layout = lymph::ObservationLayout::Rows,
                   },
                   .y = pluss::make_series(labels, {40}, f64),
               },
               options);

  std::cout << "semi rounds: " << result.rounds() << "\n";
  std::cout << "pseudo labels: " << result.pseudo_labeled_count() << "\n";
  return 0;
}
