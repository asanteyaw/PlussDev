#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 16;
  cls::HashMapper mapper(options);

  std::cout << "3 documents -> width = " << mapper.map_dense({"a", "b", "c"}).value().x.sizes()[1] << "\n";
  std::cout << "30 distinct-word documents -> width = "
            << mapper.map_dense({"aardvark", "bear", "cat", "dog", "eel", "fox", "goat", "hare",
                                  "ibis", "jay", "koala", "lynx", "mole", "newt", "owl", "pig",
                                  "quail", "rat", "seal", "toad", "urchin", "vole", "wolf", "yak",
                                  "zebu", "ant", "bee", "cow", "doe", "emu"})
                   .value()
                   .x.sizes()[1]
            << " (unchanged, unlike LexiconMapper::feature_count())\n";

  return 0;
}
