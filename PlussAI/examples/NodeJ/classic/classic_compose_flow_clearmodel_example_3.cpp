// compose::Flow::clear_model example 3 -- a Transform-task model is removed
// cleanly too, even though it is tracked internally as the transform-stage
// list's tail rather than a dedicated classifier/regressor slot.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;
  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_model(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster});
  std::cout << "before clear_model(): stage_count() = " << flow.stage_count()
            << ", has_model() = " << std::boolalpha << flow.has_model() << "\n";

  flow.clear_model();
  std::cout << "after clear_model(): stage_count() = " << flow.stage_count()
            << ", has_model() = " << flow.has_model() << "\n";

  // The prep stage added before the model survives; only the model was removed.
  const cls::Status s = flow.add_one_hot_encoder({}, {1});
  std::cout << "adding another stage afterward ok = " << s.ok() << "\n";
  std::cout << "final stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
