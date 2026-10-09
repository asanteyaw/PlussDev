#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;

  const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path().parent_path();
  pluss::loader::ReadCsvOptions csv{};
  csv.header = false;
  csv.names = {"crim", "zn", "indus", "chas", "nox", "rm", "age", "dis", "rad", "tax", "ptratio", "b", "lstat", "medv"};
  const DataFrame raw = DataFrame::load_csv((root / "data/boston_housing.csv").string(), csv);

  lymph::DataFrameBatchOptions data{};
  data.feature_names = {"crim", "zn", "indus", "chas", "nox", "rm", "age", "dis", "rad", "tax", "ptratio", "b", "lstat"};
  data.label_feature = "medv";

  const lymph::shape::Result profile = lymph::shape::data(raw, data).run({
      .skew_threshold = 0.75,
      .heavy_tail_threshold = 2.0,
  });

  lymph::models::Model model = lymph::models::linear_regression({.regularization_strength = 0.001});
  profile.plan({.skew_threshold = 0.75, .heavy_tail_threshold = 2.0}).apply(model);
  model.learn_(raw, data);

  std::cout << "boston profiled features: " << profile.feature_count() << "\n";
  std::cout << "boston gaussianized features: "
            << profile.plan({.skew_threshold = 0.75, .heavy_tail_threshold = 2.0}).feature_count() << "\n";
  std::cout << "boston shape-plan r2: " << model.assess(raw, data).r2.item<double>() << "\n";
  return 0;
}
