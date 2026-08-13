#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  ai::StepOptions shrink{};
  shrink.capabilities = {ai::StepCapability::Map};
  shrink.inputs = {"x"};
  shrink.outputs = {"short"};
  shrink.can_change_row_count = true;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("shrink", shrink, [cfg](const std::vector<ai::Artifact>&,
                                                  const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("short", ai::Artifact::Value(pluss::make_series(std::vector<double>{4.0, 5.0}, {2}, cfg)));
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "ok=" << out.ok() << "\n";
  std::cout << "changed_rows=" << out.audit().front().row_count_changed << "\n";
  std::cout << "rows=" << out.audit().front().output.rows << "\n";
  return 0;
}
