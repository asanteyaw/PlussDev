#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

struct SquareModule {
  pluss::Series forward(const pluss::Series& x) {
    return x.mul(x);
  }
};

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("square", "x", "x2", SquareModule{}),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "elements=" << out.get<pluss::Series>("x2").numel() << "\n";
  std::cout << "component=" << proc.get("square").component() << "\n";
  return 0;
}
