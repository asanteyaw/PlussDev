#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions bad{};
  bad.n_features = 0;
  cls::HashMapper mapper(bad);
  std::cout << "construction always succeeds -- no crash reaching this line\n";

  std::cout << "first map_dense call ok  = " << std::boolalpha << mapper.map_dense({"hello"}).ok() << "\n";
  std::cout << "second map_dense call ok = " << mapper.map_dense({"hello again"}).ok() << "\n";

  cls::HashMapOptions bad_ngram{};
  bad_ngram.min_ngram = 5;
  bad_ngram.max_ngram = 1;
  cls::HashMapper ngram_mapper(bad_ngram);
  std::cout << "bad ngram range ok       = " << ngram_mapper.map_dense({"hello"}).ok() << "\n";

  cls::HashMapper empty_docs_mapper;
  std::cout << "empty documents ok       = " << empty_docs_mapper.map_dense({}).ok() << "\n";

  return 0;
}
