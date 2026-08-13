#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"rate"};
  options.outputs = {"annual"};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("annualize", options, [](const std::vector<ai::Artifact>& inputs,
                                                   const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("annual", ai::Artifact::Value(inputs.front().get<double>() * 12.0));
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("rate", 0.015)});
  std::cout << "annual=" << out.get<double>("annual") << "\n";
  return 0;
}
