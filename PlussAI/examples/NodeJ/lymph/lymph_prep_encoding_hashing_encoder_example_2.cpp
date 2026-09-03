#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const DataFrame train({{"tag", Thread(pluss::make_note(
      {"red", "green", "blue", "cyan", "black", "white"}))}});

  // seed perturbs the hash: the same categories land in a different bucket
  // assignment. Widening n_components lowers the collision rate at the cost of
  // a wider frame.
  lymph::prep::HashingEncoder a(lymph::prep::HashingEncoderOptions{.n_components = 3, .seed = 0});
  lymph::prep::HashingEncoder b(lymph::prep::HashingEncoderOptions{.n_components = 3, .seed = 7});
  std::cout << "seed = 0:\n" << a.features({"tag"}).learn_map(train) << "\n";
  std::cout << "seed = 7 -- same rows, different buckets:\n"
            << b.features({"tag"}).learn_map(train) << "\n";

  return 0;
}
