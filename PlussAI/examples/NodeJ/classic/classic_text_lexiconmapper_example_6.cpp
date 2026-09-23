#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_ngram = 2;
  options.max_ngram = 2;
  cls::LexiconMapper mapper(options);

  mapper.learn({"the cat sat", "the cat ran"});
  std::cout << "vocabulary (bigrams) = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
