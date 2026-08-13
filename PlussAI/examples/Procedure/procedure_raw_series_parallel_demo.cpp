#include <pluss/ai/procedure.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 4.0, 8.0}, {4}, cfg);

  ai::StepOptions join_options{};
  join_options.capabilities = {ai::StepCapability::Join};
  join_options.inputs = {"centered", "energy"};
  join_options.outputs = {"score"};

  ai::ProcedureOptions options{};
  options.workers = 2;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("center", "x", "centered", [](const pluss::Series& input) {
        return input.sub(input.mean());
      }),
      ai::procedure::step("energy", "x", "energy", [](const pluss::Series& input) {
        return input.mul(input);
      }),
      ai::procedure::step("score", join_options, [](const std::vector<ai::Artifact>& inputs,
                                                    const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("score", ai::Artifact::Value(
            inputs[0].get<pluss::Series>().add(inputs[1].get<pluss::Series>())));
      }),
  }, {
      ai::procedure::link("center", "score", "centered", "centered"),
      ai::procedure::link("energy", "score", "energy", "energy"),
  }, options);

  const ai::ProcedureResult out = proc.run({ai::procedure::input("x", x)});
  std::cout << "score_elements=" << out.get<pluss::Series>("score").numel() << "\n";
  std::cout << "steps_ran=" << out.trace().size() << "\n";
  std::cout << "audit_records=" << out.audit().size() << "\n";
  return 0;
}
