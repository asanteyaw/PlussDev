#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// The self-describing save form: Model::save(path, x_train, y_train,
// train_options, metadata) embeds the training data and a provenance stamp
// in the bundle. io::load reads the metadata back without restoring the
// model.
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

  lymph::io::FlowBundleMetadata metadata{};
  metadata.producer = "lymph_io_save_example_3";
  metadata.producer_version = "1.4.2";

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_save_example_3.flow").string();
  model.save(path, x, y, lymph::options::Train{}, metadata);

  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "metadata.producer         = " << bundle.metadata.producer << "\n";
  std::cout << "metadata.producer_version = " << bundle.metadata.producer_version << "\n";
  std::cout << "bundle.trained            = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "bundle.has_snapshot       = " << bundle.has_snapshot << "\n";

  std::filesystem::remove(path);
  return 0;
}
