#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// n_components is the width / collision trade-off. Six distinct tags hashed into
// 2 buckets must collide heavily; into 8 buckets they spread out. The encoded
// matrices show it directly.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const DataFrame train({{"tag", Slate(pluss::make_note(
      {"red", "green", "blue", "cyan", "black", "white"}))}});

  lymph::prep::HashingEncoder narrow{lymph::prep::HashingEncoderOptions{.n_components = 2}};
  std::cout << "n_components = 2 -- heavy collisions, only 2 possible rows:\n"
            << narrow.features({"tag"}).learn_map(train) << "\n";

  lymph::prep::HashingEncoder wide{lymph::prep::HashingEncoderOptions{.n_components = 8}};
  std::cout << "n_components = 8 -- room to separate the six tags:\n"
            << wide.features({"tag"}).learn_map(train) << "\n";

  return 0;
}
