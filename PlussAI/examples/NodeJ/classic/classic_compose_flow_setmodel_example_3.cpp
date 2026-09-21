// compose::Flow::set_model example 3 -- ordering matters for a
// Transform-task spec: add every transform stage first, then set_model
// last. Reversing the order is now a rejected precondition, not a silent
// misroute (a real bug found and fixed while writing this reference).

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;
  const cls::ModelSpec spec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster};

  // Safe order: stage(s) first, model last.
  cls::Flow ok_order{};
  ok_order.add_standard_scaler({}, {0});
  const cls::Status s1 = ok_order.set_model(spec);
  std::cout << "stage-then-model ok = " << std::boolalpha << s1.ok() << "\n";
  std::cout << "stage-then-model stage_count() = " << ok_order.stage_count() << "\n";

  // Unsafe order: model first, then a stage -- rejected.
  cls::Flow bad_order{};
  bad_order.set_model(spec);
  const cls::Status s2 = bad_order.add_standard_scaler({}, {0});
  std::cout << "model-then-stage ok = " << s2.ok() << "\n";
  std::cout << "model-then-stage message = \"" << s2.message << "\"\n";

  return 0;
}
