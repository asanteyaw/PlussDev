#include <pluss/ai/procedure.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;

  ai::ProcedureOptions options{};
  options.workers = 3;
  options.schedule = ai::ProcedureSchedule::Adaptive;
  options.cache = ai::ProcedureCache::ReadyArtifacts;

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("flag", ai::StepOptions{
          .capabilities = {ai::StepCapability::Map},
          .outputs = {"ok"},
      }, [](const std::vector<ai::Artifact>&, const ai::RunOptions&) -> ai::Artifact {
        return ai::Artifact("ok", ai::Artifact::Value(true));
      }),
  }, {}, options);

  std::cout << "workers=" << proc.options().workers << "\n";
  std::cout << "schedule=" << static_cast<std::int64_t>(proc.options().schedule) << "\n";
  std::cout << "cache=" << static_cast<std::int64_t>(proc.options().cache) << "\n";
  return 0;
}
