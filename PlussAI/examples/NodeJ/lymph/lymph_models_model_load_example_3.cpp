#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;

  // load fails loudly when the path does not exist or does not hold a valid
  // bundle. Handle it the way you would any other I/O error.
  try {
    const lymph::models::Model model =
        lymph::models::Model::load("/tmp/does-not-exist-lymph.flow");
    std::cout << "unexpectedly loaded a model\n";
  } catch (const std::exception& err) {
    std::cout << "load threw: " << err.what() << "\n";
  }

  return 0;
}
