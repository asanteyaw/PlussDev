#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LexiconMapperState mismatched{};
  mismatched.vocabulary = {"a", "b"};
  mismatched.document_frequency = {1};
  cls::LexiconMapper x;
  std::cout << "length mismatch ok       = " << std::boolalpha << x.load_state(mismatched).ok() << "\n";

  cls::LexiconMapperState empty_but_learned{};
  empty_but_learned.learned = true;
  cls::LexiconMapper y;
  std::cout << "learned=true, empty vocab ok = " << y.load_state(empty_but_learned).ok() << "\n";

  return 0;
}
