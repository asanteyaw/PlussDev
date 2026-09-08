#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// The bundle schema is versioned. io::load reports the version it read, and
// it rejects a file whose schema_version does not match the current one
// outright -- there is no tolerant reading of an older format, so a bundle
// from an earlier build must be re-saved.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series a = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 1}, {4}, f64);
  const DataFrame x({{"a", Thread(a)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_3.flow").string();
  model.save(path);

  const lymph::io::FlowBundle ok = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "current schema_version = " << ok.metadata.schema_version << "\n";

  // Loading a path that is not a bundle at all fails, not silently returns junk.
  const std::string bogus =
      (std::filesystem::temp_directory_path() / "lymph_io_persistence_example_3_missing.flow").string();
  const auto result = lymph::io::load(bogus);
  std::cout << "load(missing).ok() = " << std::boolalpha << result.ok() << "\n";

  std::filesystem::remove(path);
  return 0;
}
