#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Learn};
  options.inputs = {"x"};
  options.outputs = {"ok"};
  options.learns_from_future = true;

  ai::ProcedureOptions proc_options{};
  proc_options.fail_fast = false;

  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("future_feature", options, [](const std::vector<ai::Artifact>&,
                                                        const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("ok", ai::Artifact::Value(true));
      }),
  }, {}, proc_options);

  const ai::ProcedureResult blocked = proc.run({ai::procedure::input("x", x)}, learn);
  learn.allow_temporal_leakage = true;
  const ai::ProcedureResult allowed = proc.run({ai::procedure::input("x", x)}, learn);
  std::cout << "blocked_ok=" << blocked.ok() << "\n";
  std::cout << "allowed_ok=" << allowed.ok() << "\n";
  return 0;
}
