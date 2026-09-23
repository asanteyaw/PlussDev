#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper counts;
  counts.learn({"cat cat cat dog"});

  cls::LexiconMapOptions binary_options{};
  binary_options.binary = true;
  cls::LexiconMapper presence(binary_options);
  presence.learn({"cat cat cat dog"});

  std::cout << "counts    = " << counts.map_dense({"cat cat cat dog"}).value().x << "\n";
  std::cout << "presence  = " << presence.map_dense({"cat cat cat dog"}).value().x << "\n";

  return 0;
}
