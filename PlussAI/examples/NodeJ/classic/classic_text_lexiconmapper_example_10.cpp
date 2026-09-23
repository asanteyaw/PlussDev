#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper mapper;
  mapper.learn({"cat sat", "dog ran"});

  cls::TfIdfMapOptions linear{};
  linear.l2_normalize = false;
  cls::TfIdfMapOptions sublinear{};
  sublinear.sublinear_tf = true;
  sublinear.l2_normalize = false;

  // "cat" repeats 5 times: linear tf = 5, sublinear tf = 1 + ln(5) = 2.6094379.
  std::cout << "linear tf    = " << mapper.map_tfidf_dense({"cat cat cat cat cat"}, linear).value().x << "\n";
  std::cout << "sublinear tf = " << mapper.map_tfidf_dense({"cat cat cat cat cat"}, sublinear).value().x << "\n";

  return 0;
}
