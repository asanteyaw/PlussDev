#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.max_features = 2;
  cls::LexiconMapper mapper(options);

  // "the" appears in 3 docs; "cat" and "dog" each appear in 2 (a tie,
  // broken alphabetically: "cat" before "dog"); "ran"/"sat" each in 1.
  mapper.learn({"the cat sat", "the cat ran", "the dog sat", "a dog ran too"});
  std::cout << "vocabulary (kept, in order) = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
