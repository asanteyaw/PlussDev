#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::procedure::bind("procedure.demo.square", [](const pluss::Series& input) {
    return input.mul(input);
  });

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, cfg);
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("square", "x", "y", [](const pluss::Series& input) {
        return input.mul(input);
      }, "procedure.demo.square"),
  });

  const std::filesystem::path bundle =
      std::filesystem::temp_directory_path() / "plussai_procedure_registry_persistence_demo.bundle";
  proc.save(bundle.string());
  const ai::Procedure restored = ai::procedure::load(bundle.string());
  const ai::ProcedureResult out = restored.run({ai::procedure::input("x", x)});
  std::filesystem::remove_all(bundle);

  std::cout << "squared_elements=" << out.get<pluss::Series>("y").numel() << "\n";
  std::cout << "restored_steps=" << restored.step_count() << "\n";
  return 0;
}
