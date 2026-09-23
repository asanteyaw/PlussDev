#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "cat ran", "dog sat", "dog ran"});
  // "cat" has document_frequency = 2, corpus n_docs = 4.

  cls::TfIdfMapOptions smoothed{};
  smoothed.l2_normalize = false;
  cls::TfIdfMapOptions unsmoothed{};
  unsmoothed.smooth_idf = false;
  unsmoothed.l2_normalize = false;

  std::cout << "smoothed   (expect idf(cat) = ln(5/3)+1 = 1.5108256) = "
            << mapper.map_tfidf_dense({"cat"}, smoothed).value().x << "\n";
  std::cout << "unsmoothed (expect idf(cat) = ln(4/2)+1 = 1.6931472) = "
            << mapper.map_tfidf_dense({"cat"}, unsmoothed).value().x << "\n";

  return 0;
}
