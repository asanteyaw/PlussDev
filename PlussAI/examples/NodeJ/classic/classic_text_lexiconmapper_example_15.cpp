#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper empty_docs;
  std::cout << "empty documents ok      = " << std::boolalpha << empty_docs.learn({}).ok() << "\n";

  cls::LexiconMapOptions zero_features{};
  zero_features.max_features = 0;
  cls::LexiconMapper a(zero_features);
  std::cout << "max_features=0 ok       = " << a.learn({"one two"}).ok() << "\n";

  cls::LexiconMapOptions zero_dfloor{};
  zero_dfloor.min_document_frequency = 0;
  cls::LexiconMapper b(zero_dfloor);
  std::cout << "min_document_frequency=0 ok = " << b.learn({"one two"}).ok() << "\n";

  cls::LexiconMapOptions bad_ngram{};
  bad_ngram.min_ngram = 3;
  bad_ngram.max_ngram = 1;
  cls::LexiconMapper c(bad_ngram);
  std::cout << "min_ngram>max_ngram ok  = " << c.learn({"one two three"}).ok() << "\n";

  cls::LexiconMapOptions impossible_floor{};
  impossible_floor.min_document_frequency = 5;
  cls::LexiconMapper d(impossible_floor);
  const cls::Status floor_status = d.learn({"a b", "c d"});
  std::cout << "unreachable floor ok    = " << floor_status.ok() << "\n";
  std::cout << "unreachable floor message = \"" << floor_status.message << "\"\n";

  return 0;
}
