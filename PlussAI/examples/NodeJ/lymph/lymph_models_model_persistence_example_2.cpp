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

  lymph::models::Model model = lymph::models::logistic_regression();
  model.prepare().scale({"income", "balance"});
  model.learn_(x, y);

  // The overloads that also take the training data embed that batch in the
  // bundle, so the saved artifact is self-describing: it records what it was
  // fitted on, not only the fitted parameters.
  const std::filesystem::path path = std::filesystem::temp_directory_path() / "lymph_model_with_data.flow";
  model.save(path.string(), x, y);

  const std::uintmax_t plain_size = [&] {
    const std::filesystem::path p2 = std::filesystem::temp_directory_path() / "lymph_model_plain.flow";
    model.save(p2.string());
    const std::uintmax_t s = std::filesystem::file_size(p2);
    std::filesystem::remove(p2);
    return s;
  }();

  std::cout << "bundle without data = " << plain_size << " bytes\n";
  std::cout << "bundle with data    = " << std::filesystem::file_size(path) << " bytes\n";

  const lymph::models::Model reloaded = lymph::models::Model::load(path.string());
  std::cout << "reloaded trained = " << std::boolalpha << reloaded.trained()
            << ", stage_count = " << reloaded.stage_count() << "\n";

  std::filesystem::remove(path);
  return 0;
}
