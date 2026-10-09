#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// models::Model::save(path) -- the everyday model write path. The proof that
// matters is the round-trip: reload with Model::load and the predictions are
// identical.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_1.flow").string();
  model.save(path);
  std::cout << "file written = " << std::boolalpha << std::filesystem::exists(path) << "\n";

  const lymph::models::Model loaded = lymph::models::Model::load(path);
  std::cout << "loaded.trained() = " << loaded.trained() << "\n";
  std::cout << "original = " << model.infer(x) << "\n";
  std::cout << "loaded   = " << loaded.infer(x) << "\n";

  std::filesystem::remove(path);
  return 0;
}
