#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  ai::StepOptions join{};
  join.capabilities = {ai::StepCapability::Join};
  join.inputs = {"a", "b", "c"};
  join.outputs = {"sum"};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("a", "x", "a", [](const pluss::Series& s) { return s; }),
      ai::procedure::step("b", "x", "b", [](const pluss::Series& s) { return s.mul(2.0); }),
      ai::procedure::step("c", "x", "c", [](const pluss::Series& s) { return s.mul(3.0); }),
      ai::procedure::step("sum", join, [](const std::vector<ai::Artifact>& inputs,
                                          const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("sum", ai::Artifact::Value(
            inputs[0].get<pluss::Series>().add(inputs[1].get<pluss::Series>()).add(inputs[2].get<pluss::Series>())));
      }),
  }, {
      ai::procedure::link("a", "sum", "a", "a"),
      ai::procedure::link("b", "sum", "b", "b"),
      ai::procedure::link("c", "sum", "c", "c"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "sum_elements=" << out.get<pluss::Series>("sum").numel() << "\n";
  std::cout << "inputs=" << proc.get("sum").inputs().size() << "\n";
  return 0;
}
