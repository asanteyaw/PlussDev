#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow named;
  named.add_standard_scaler();
  std::cout << "named stage:  has_opaque_transforms = " << std::boolalpha
            << named.graph_info().value().has_opaque_transforms << "\n";

  cls::Flow opaque;
  opaque.append_transform(std::make_unique<cls::prep::StandardScaler>());
  std::cout << "append_transform: has_opaque_transforms = "
            << opaque.graph_info().value().has_opaque_transforms << "\n";

  return 0;
}
