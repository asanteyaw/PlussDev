#include <pluss/ai/procedure.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame good({
      {"timestamp", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg))},
      {"x", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });
  const DataFrame missing_time({
      {"x", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3}, cfg))},
  });

  ai::StepOptions gate{};
  gate.capabilities = {ai::StepCapability::Gate, ai::StepCapability::Map};
  gate.inputs = {"raw"};
  gate.outputs = {"clean"};
  gate.require_columns = {"timestamp", "x"};
  gate.time_feature = "timestamp";
  gate.requires_time = true;

  ai::ProcedureOptions options{};
  options.fail_fast = false;
  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("gate", gate, [](const std::vector<ai::Artifact>& inputs,
                                           const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("clean", ai::Artifact::Value(inputs.front().get<DataFrame>()));
      }),
  }, {}, options);

  const ai::ProcedureResult ok = proc.run({ai::procedure::input("raw", good)});
  const ai::ProcedureResult blocked = proc.run({ai::procedure::input("raw", missing_time)});

  std::cout << "valid_ok=" << ok.ok() << "\n";
  std::cout << "blocked_ok=" << blocked.ok() << "\n";
  std::cout << "blocked_step=" << blocked.failed_step() << "\n";
  std::cout << "warnings=" << blocked.warnings().size() << "\n";
  return 0;
}
