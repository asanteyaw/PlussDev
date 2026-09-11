// families::ModelSpec example 1 -- one spec per family shape, matched to the
// right make_* function.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec linear_clf{cls::TaskKind::Classification, cls::ModelFamily::Linear,
                                  cls::LinearFamilyOptions{}};
  const cls::ModelSpec tree_reg{cls::TaskKind::Regression, cls::ModelFamily::Tree,
                                cls::TreeFamilyOptions{}};
  const cls::ModelSpec kmeans{cls::TaskKind::Transform, cls::ModelFamily::Cluster,
                              cls::ClusterFamilyOptions{}};

  std::cout << "linear classifier made ok = " << std::boolalpha
            << cls::make_classifier(linear_clf).ok() << "\n";
  std::cout << "tree regressor made ok    = " << cls::make_regressor(tree_reg).ok() << "\n";
  std::cout << "kmeans transformer made ok = " << cls::make_transformer(kmeans).ok() << "\n";

  return 0;
}
