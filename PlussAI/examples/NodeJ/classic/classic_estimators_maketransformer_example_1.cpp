// estimators::make_transformer example 1 -- the four transformer-capable
// families.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec kmeans{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cls::ClusterFamilyOptions{}};
  const cls::ModelSpec pca{cls::TaskKind::Transform, cls::ModelFamily::Decompose, cls::DecomposeFamilyOptions{}};
  const cls::ModelSpec isolation{cls::TaskKind::Transform, cls::ModelFamily::Detect, cls::DetectFamilyOptions{}};

  cls::KernelFamilyOptions one_class{};
  one_class.form = cls::KernelForm::OneClass;
  const cls::ModelSpec kernel{cls::TaskKind::Transform, cls::ModelFamily::Kernel, one_class};

  std::cout << "kmeans ok    = " << std::boolalpha << cls::make_transformer(kmeans).ok() << "\n";
  std::cout << "pca ok       = " << cls::make_transformer(pca).ok() << "\n";
  std::cout << "isolation ok = " << cls::make_transformer(isolation).ok() << "\n";
  std::cout << "one-class svm ok = " << cls::make_transformer(kernel).ok() << "\n";

  return 0;
}
