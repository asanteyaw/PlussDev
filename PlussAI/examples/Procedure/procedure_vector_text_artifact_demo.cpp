#include <pluss/ai/procedure.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::StepOptions options{};
  options.capabilities = {ai::StepCapability::Map};
  options.inputs = {"texts"};
  options.outputs = {"count"};

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("count_docs", options, [](const std::vector<ai::Artifact>& inputs,
                                                    const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("count", ai::Artifact::Value(static_cast<std::int64_t>(
            inputs.front().get<std::vector<std::string>>().size())));
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("texts", std::vector<std::string>{"one", "two"})});
  std::cout << "count=" << out.get<std::int64_t>("count") << "\n";
  return 0;
}
