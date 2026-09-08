#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

// io::load(path) is the low-level read: it returns a FlowBundle you can
// inspect (schema version, trained flag, transform count) without restoring
// anything. workflow::restore(bundle, policy) then rebuilds a Workflow with
// a Policy you choose -- not the one from a sidecar.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Thread(income)}, {"balance", Thread(balance)}});

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_io_load_example_3.flow").string();
  {
    lymph::models::Model model = lymph::models::logistic_regression();
    lymph::models::ModelPrep prep = model.prepare();
    prep.scale({"income", "balance"});
    model.learn_(x, y);
    model.save(path);
  }

  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "io::load");
  std::cout << "schema_version   = " << bundle.metadata.schema_version << "\n";
  std::cout << "bundle.trained   = " << std::boolalpha << bundle.trained << "\n";
  std::cout << "transform count  = " << bundle.transforms.size() << "\n";

  // Rebuild as a Workflow with a chosen Policy.
  workflow::Policy policy{};
  policy.require({"income", "balance"});
  const workflow::Workflow restored = workflow::restore(bundle, policy);
  std::cout << "restored.trained() = " << restored.trained()
            << "  policy require count = " << restored.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);
  return 0;
}
