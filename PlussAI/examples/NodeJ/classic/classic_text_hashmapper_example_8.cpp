#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 32;
  options.alternate_sign = true;

  cls::HashMapper first(options);
  cls::HashMapper second(options);

  const double diff = (first.map_dense({"the cat sat", "the dog ran"}).value().x
                        - second.map_dense({"the cat sat", "the dog ran"}).value().x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between two independent instances = " << diff << "\n";

  return 0;
}
