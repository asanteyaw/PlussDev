#include <pluss/ai/procedure.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("store", ai::procedure::sink("demo.empty")),
  });

  const ai::ProcedureResult out = proc.run();
  std::cout << "ok=" << out.ok() << "\n";
  std::cout << "empty=" << out.artifacts().front().empty() << "\n";
  return 0;
}
