// A default-constructed batch has no fields. empty() and size() agree about that, and x() refuses
// to read a field that is not there.
#include <pluss/tils/kit/data_loader.h>
#include <pluss/pluss.h>

#include <exception>
#include <iostream>

int main() {
  std::cout << std::boolalpha;
  namespace loader = pluss::loader;
  const loader::DataBatch empty_batch{};
  std::cout << "empty: " << empty_batch.empty() << '\n';
  std::cout << "size: " << empty_batch.size() << '\n';
  bool refused = false;
  try {
    (void)empty_batch.x();
  } catch (const std::exception&) {
    refused = true;
  }
  std::cout << "reading x on an empty batch refused: " << refused << '\n';
  return 0;
}
