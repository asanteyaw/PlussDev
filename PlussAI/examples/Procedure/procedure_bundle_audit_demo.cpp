#include <pluss/ai/procedure.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"drop_me", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg))},
      {"x", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}, {2}, cfg))},
  });
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step(
          "clean",
          "raw",
          "frame",
          lymph::workflow::Policy{.drop_columns = {"drop_me"}}),
      ai::procedure::step("out", ai::procedure::sink("procedure.demo.frame")),
  }, {
      ai::procedure::link("clean", "out"),
  });

  const std::filesystem::path bundle =
      std::filesystem::temp_directory_path() / "plussai_procedure_bundle_audit_demo.bundle";
  proc.save(bundle.string());
  const ai::Procedure restored = ai::procedure::load(bundle.string());
  const ai::ProcedureResult out = restored.run({ai::procedure::input("raw", raw)});

  std::ifstream summary_file(bundle / "audit" / "summary.txt");
  const std::string summary((std::istreambuf_iterator<char>(summary_file)), std::istreambuf_iterator<char>());
  std::filesystem::remove_all(bundle);

  std::cout << "restored_steps=" << restored.step_count() << "\n";
  std::cout << "output_columns=" << out.get<DataFrame>("frame").nfeatures() << "\n";
  std::cout << "summary_bytes=" << summary.size() << "\n";
  return 0;
}
