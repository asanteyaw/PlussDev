#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "cat ran", "dog sat", "dog ran"});

  cls::TfIdfMapOptions options{};
  options.l2_normalize = false;
  const pluss::Series held_out = mapper.map_tfidf_dense({"cat appeared in a brand new document"}, options).value().x;
  std::cout << "held-out document's TF-IDF row (cat's idf still from training) = " << held_out << "\n";

  return 0;
}
