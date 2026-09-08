#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// What round-trips: fitted prep-stage state. A model with a standard scaler
// carries the scaler's learned centre/spread into the bundle; after
// save/load, map() produces the identical transformed matrix.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"a", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64))},
      {"b", Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64))},
  });
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.prepare().scale({"a", "b"});
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_1.flow").string();
  model.save(path);
  const lymph::models::Model loaded = lymph::models::Model::load(path);

  std::cout << "original map =\n" << model.map(x) << "\n";
  std::cout << "loaded   map =\n" << loaded.map(x) << "\n";
  std::cout << "loaded.stage_count() = " << loaded.stage_count() << "\n";

  std::filesystem::remove(path);
  return 0;
}
