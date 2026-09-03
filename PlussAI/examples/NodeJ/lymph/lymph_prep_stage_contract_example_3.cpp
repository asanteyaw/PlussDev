#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>

int main() {
  namespace lymph = pluss::ai::lymph;

  // stateful tells you whether a stage must be *fitted* on training data
  // (and its learned state replayed at inference) or is a pure function that
  // behaves identically on every frame.
  const std::pair<const char*, lymph::prep::Stage> stages[] = {
      {"standard_scaler", lymph::prep::stage::standard_scaler()},
      {"quantile_map", lymph::prep::stage::quantile_map()},
      {"vector_normalizer", lymph::prep::stage::vector_normalizer()},
      {"threshold_binarizer", lymph::prep::stage::threshold_binarizer()},
  };

  for (const auto& [label, stage] : stages) {
    const lymph::prep::TransformContract c = stage.contract();
    std::cout << label << ": "
              << (c.stateful ? "fitted on training data"
                             : "pure function, no fitting")
              << (c.needs_inverse_memory ? " (keeps inverse memory)" : "")
              << "\n";
  }

  return 0;
}
