#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.fail_fast = false;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("needs_signal", "signal", "out", [](const pluss::Series& s) {
        return s;
      }),
  }, {}, options);

  const ai::ProcedureResult out = proc.run();
  std::cout << "ok=" << out.ok() << "\n";
  std::cout << "failed_step=" << out.failed_step() << "\n";
  std::cout << "warnings=" << out.warnings().size() << "\n";
  return 0;
}
