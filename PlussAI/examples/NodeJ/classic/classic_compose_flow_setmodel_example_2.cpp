// compose::Flow::set_model example 2 -- a Transform-task ModelSpec builds a
// real transformer, tracked internally as the transform-stage list's tail
// rather than a dedicated classifier/regressor slot -- but has_model() and
// stage_count() both account for it correctly.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;

  cls::Flow flow{};
  const cls::Status s = flow.set_model(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster});
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "task_kind() == Transform: " << (flow.task_kind() == cls::TaskKind::Transform) << "\n";
  std::cout << "stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
