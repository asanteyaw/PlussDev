#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>

int main() {
  namespace lymph = pluss::ai::lymph;

  // contract() answers questions about a stage kind without running it: the
  // metadata you need to audit a preprocessing plan rather than execute it.
  const std::pair<const char*, lymph::prep::Stage> stages[] = {
      {"standard_scaler", lymph::prep::stage::standard_scaler()},
      {"one_hot_encoder", lymph::prep::stage::one_hot_encoder()},
      {"simple_imputer", lymph::prep::stage::simple_imputer()},
      {"polynomial_lift", lymph::prep::stage::polynomial_lift()},
      {"threshold_binarizer", lymph::prep::stage::threshold_binarizer()},
  };

  for (const auto& [label, stage] : stages) {
    const lymph::prep::TransformContract c = stage.contract();
    std::cout << label
              << ": stateful=" << std::boolalpha << c.stateful
              << " lossy=" << c.lossy
              << " reversible=" << c.reversible
              << " changes_feature_count=" << c.changes_feature_count
              << " changes_sample_count=" << c.changes_sample_count << "\n";
  }

  return 0;
}
