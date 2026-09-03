#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // A FlowBundleMetadata stamp travels with the bundle: which component wrote
  // it and at what version. Set the producer fields for your own service so a
  // reloaded artifact can be traced back to the run that made it.
  lymph::io::FlowBundleMetadata meta{};
  meta.producer = "credit-scoring-service";
  meta.producer_version = "2.4.1";

  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model_stamped.flow";
  model.save(path.string(), meta);

  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  std::cout << "reloaded trained = " << std::boolalpha << reloaded.trained() << "\n\n";

  const DataFrame check({
      {"income", Thread(income)},
      {"original", Thread(model.infer(x))},
      {"reloaded", Thread(reloaded.infer(x))},
  });
  std::cout << check;

  std::filesystem::remove(path);
  return 0;
}
