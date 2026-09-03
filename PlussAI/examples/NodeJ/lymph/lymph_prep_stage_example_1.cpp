#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // A Stage is a value: a transform kind plus that transform's options. The
  // prep::stage::* factories are the readable way to make one. Nothing here
  // touches data -- these are descriptors, not fitted objects.
  const lymph::prep::Stage scaler = lymph::prep::stage::standard_scaler();
  const lymph::prep::Stage encoder = lymph::prep::stage::one_hot_encoder();
  const lymph::prep::Stage lift = lymph::prep::stage::polynomial_lift();

  // contract() reports what the kind does, without running it -- the data you
  // need to audit a prep graph rather than execute it.
  for (const auto& [label, stage] : {std::pair{"standard_scaler", scaler},
                                     std::pair{"one_hot_encoder", encoder},
                                     std::pair{"polynomial_lift", lift}}) {
    const lymph::prep::TransformContract c = stage.contract();
    std::cout << label << ": stateful=" << std::boolalpha << c.stateful
              << " lossy=" << c.lossy
              << " reversible=" << c.reversible
              << " changes_feature_count=" << c.changes_feature_count << "\n";
  }

  return 0;
}
