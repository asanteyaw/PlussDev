#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapOptions options{};
  options.min_ngram = 3;
  options.max_ngram = 3;
  cls::LexiconMapper mapper(options);

  // "ok" (1 token) and "a b" (2 tokens) are both too short for a trigram;
  // "the cat sat down" (4 tokens) contributes two trigrams.
  const cls::Status learned = mapper.learn({"the cat sat down", "ok", "a b"});
  std::cout << "learn ok = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "feature_count() = " << mapper.feature_count() << "\n";

  return 0;
}
