#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "dog ran", "cat ran"});

  cls::TfIdfMapOptions normalized{};
  cls::TfIdfMapOptions raw{};
  raw.l2_normalize = false;

  const pluss::Series normalized_row = mapper.map_tfidf_dense({"cat ran fast"}, normalized).value().x;
  const pluss::Series raw_row = mapper.map_tfidf_dense({"cat ran fast"}, raw).value().x;
  std::cout << "normalized row L2 norm = "
            << normalized_row.square().sum(1, false).sqrt().item<double>() << "\n";
  std::cout << "raw row L2 norm        = "
            << raw_row.square().sum(1, false).sqrt().item<double>() << "\n";

  return 0;
}
