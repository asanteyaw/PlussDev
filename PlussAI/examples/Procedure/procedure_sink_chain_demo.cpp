#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{4.0, 5.0}, {2}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("double", "x", "y", [](const pluss::Series& s) {
        return s.mul(2.0);
      }),
      ai::procedure::step("artifact", ai::procedure::sink("demo.memory")),
  }, {
      ai::procedure::link("double", "artifact"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "sink_elements=" << out.get<pluss::Series>("artifact").numel() << "\n";
  std::cout << "sink_persistable=" << proc.get("artifact").persistable() << "\n";
  return 0;
}
