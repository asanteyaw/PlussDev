#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("flag", ai::StepOptions{
          .capabilities = {ai::StepCapability::Map},
          .outputs = {"accepted"},
      }, [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("accepted", ai::Artifact::Value(true));
      }),
  });

  const ai::ProcedureResult out = proc.run();
  std::cout << "ok=" << out.ok() << "\n";
  std::cout << "accepted=" << out.get<bool>("accepted") << "\n";
  std::cout << "artifacts=" << out.artifacts().size() << "\n";
  return 0;
}
