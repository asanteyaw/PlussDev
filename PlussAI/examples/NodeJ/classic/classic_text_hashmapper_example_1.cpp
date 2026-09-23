#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 16;
  cls::HashMapper mapper(options);

  const cls::FeatureBatch batch = mapper.map_dense({"the cat sat", "the dog ran"}).value();
  std::cout << "shape = [" << batch.x.sizes()[0] << ", " << batch.x.sizes()[1] << "]\n";
  std::cout << "matrix = " << batch.x << "\n";

  return 0;
}
