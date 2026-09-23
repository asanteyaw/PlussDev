#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"the cat sat"});

  const cls::FeatureBatch batch = mapper.map_dense({"the cat flew to jupiter"}).value();
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";
  std::cout << "map_dense (unseen terms ignored) = " << batch.x << "\n";

  return 0;
}
