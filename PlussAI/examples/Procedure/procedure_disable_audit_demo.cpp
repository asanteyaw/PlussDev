#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);

  ai::RunOptions run{};
  run.audit = false;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("double", "x", "y", [](const pluss::Series& s) {
        return s.mul(2.0);
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)}, run);
  std::cout << "trace=" << out.trace().size() << "\n";
  std::cout << "audit=" << out.audit().size() << "\n";
  return 0;
}
