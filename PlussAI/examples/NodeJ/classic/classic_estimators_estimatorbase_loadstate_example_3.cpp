// estimators::EstimatorBase::load_state example 3 -- a second family's own
// shape validation (a TransformerBase family this time, not a Classifier).

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster,
                                           cls::ClusterFamilyOptions{}});

  const std::vector<pluss::Series> state{};
  const cls::Status loaded = made.value()->load_state(state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
