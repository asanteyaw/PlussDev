#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_document_frequency = 2;
  cls::LexiconMapper mapper(options);

  // "unique" appears in exactly one document; "the"/"cat" appear in two+.
  mapper.learn({"the cat sat", "the cat ran", "a unique word appears once"});
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";
  std::cout << "vocabulary = [";
  for (const auto& term : mapper.vocabulary()) std::cout << "\"" << term << "\" ";
  std::cout << "]\n";

  return 0;
}
