#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <utility>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A plan under review: several stages, and a question -- how many of them
  // will change the width of the frame before it reaches the estimator?
  const std::vector<std::pair<const char*, lymph::prep::Stage>> plan = {
      {"scale amount", lymph::prep::stage::standard_scaler()},
      {"encode region", lymph::prep::stage::one_hot_encoder()},
      {"lift interactions", lymph::prep::stage::polynomial_lift()},
      {"scale ratio", lymph::prep::stage::minmax_scaler()},
  };

  int widening = 0;
  for (const auto& [note, stage] : plan) {
    const bool changes_width = stage.contract().changes_feature_count;
    std::cout << (changes_width ? "[width +] " : "[width =] ") << note << "\n";
    if (changes_width) ++widening;
  }
  std::cout << "\nstages that change feature count: " << widening
            << " of " << plan.size() << "\n";

  return 0;
}
