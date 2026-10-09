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

  // Train in one place, persist, then load elsewhere.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_round_trip.flow";
  lymph::models::logistic_regression().learn(x, y).save(path.string());

  // load is a static factory: it returns a fully fitted Model read back from
  // the bundle. There is no separate "trained yet?" step and no re-fit.
  const lymph::models::Model model = lymph::models::Model::load(path.string());
  std::cout << "trained right after load = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame result({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << result;

  std::filesystem::remove(path);
  return 0;
}
