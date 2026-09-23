#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapper original;
  original.learn({"the cat sat", "the dog ran", "cats and dogs"});
  const cls::LexiconMapperState state = original.dump_state();
  std::cout << "dumped vocabulary size = " << state.vocabulary.size() << "\n";

  cls::LexiconMapper restored;
  const cls::Status loaded = restored.load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";

  const double diff = (original.map_dense({"the cat ran"}).value().x
                        - restored.map_dense({"the cat ran"}).value().x)
                           .abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff after restore = " << diff << "\n";

  return 0;
}
