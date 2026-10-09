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

  const pluss::Series income = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}, {"region", Slate(region)}});

  // A model with two preparation stages plus the estimator.
  lymph::models::Model original = lymph::models::logistic_regression();
  original.prepare().scale({"income"}).encode({"region"});
  original.learn_(x, y);

  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_staged.flow";
  original.save(path.string());

  // load restores the whole flow, preparation stages included, so the loaded
  // model transforms raw inputs exactly as the original did.
  const lymph::models::Model loaded = lymph::models::Model::load(path.string());
  std::cout << "original stage_count = " << original.stage_count() << "\n";
  std::cout << "loaded   stage_count = " << loaded.stage_count() << "\n\n";

  const DataFrame check({
      {"income", Slate(income)},
      {"region", Slate(region)},
      {"original", Slate(original.infer(x))},
      {"loaded", Slate(loaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
