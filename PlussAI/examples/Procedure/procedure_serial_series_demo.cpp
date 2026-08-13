#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("scale", "x", "scaled", [](const pluss::Series& s) {
        return s.mul(10.0);
      }),
      ai::procedure::step("shift", "scaled", "score", [](const pluss::Series& s) {
        return s.add(2.0);
      }),
  }, {
      ai::procedure::link("scale", "shift"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "score_elements=" << out.get<pluss::Series>("score").numel() << "\n";
  std::cout << "steps=" << out.trace().size() << "\n";
  return 0;
}
