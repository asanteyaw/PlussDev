#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  const cls::Status learned = mapper.learn(
      {"the cat sat", "the dog sat", "cats and dogs", "the cat ran"});
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "learned() = " << mapper.learned() << "\n";
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";

  const cls::FeatureBatch batch = mapper.map_dense({"the cat sat again"}).value();
  std::cout << "map_dense = " << batch.x << "\n";

  return 0;
}
