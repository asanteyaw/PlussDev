#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // The one-argument form writes the fitted flow -- preparation state and
  // estimator parameters -- to a single bundle file.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model.flow";
  model.save(path.string());
  std::cout << "wrote " << std::filesystem::file_size(path) << " bytes\n";

  // Reload it into a brand new Model and confirm predictions are unchanged.
  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  const DataFrame check({
      {"income", Slate(income)},
      {"original", Slate(model.infer(x))},
      {"reloaded", Slate(reloaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
