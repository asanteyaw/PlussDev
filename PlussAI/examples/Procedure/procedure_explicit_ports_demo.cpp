#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{2.0, 4.0, 6.0}, {3}, cfg);

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("normalize", "raw_x", "feature_x", [](const pluss::Series& s) {
        return s.div(s.amax());
      }),
      ai::procedure::step("score", "model_input", "score", [](const pluss::Series& s) {
        return s.add(1.0);
      }),
  }, {
      ai::procedure::link("normalize", "score", "feature_x", "model_input"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw_x", x)});
  std::cout << "score_elements=" << out.get<pluss::Series>("score").numel() << "\n";
  std::cout << "links=" << proc.link_count() << "\n";
  return 0;
}
