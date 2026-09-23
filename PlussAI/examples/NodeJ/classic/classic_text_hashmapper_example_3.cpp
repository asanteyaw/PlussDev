#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::HashMapOptions options{};
  options.n_features = 3;
  cls::HashMapper mapper(options);

  for (const std::string& term : {"cat", "dog", "bird", "fish", "lion", "wolf"}) {
    const pluss::Series row = mapper.map_dense({term}).value().x;
    std::cout << term << " -> " << row << "\n";
  }

  return 0;
}
