#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  // HashingEncoder maps each category into one of n_components fixed buckets
  // with a multiplicative hash and emits an n_components-wide indicator. Output
  // width is independent of cardinality and no vocabulary is stored.
  const DataFrame train({{"tag", Thread(pluss::make_note(
      {"red", "green", "blue", "red", "cyan", "green", "black", "blue"}))}});
  const DataFrame later({{"tag", Thread(pluss::make_note({"blue", "magenta"}))}});

  lymph::prep::HashingEncoder encoder(
      lymph::prep::HashingEncoderOptions{.n_components = 4});
  std::cout << "learn_map(train) -- 4 bucket columns; equal categories may collide:\n"
            << encoder.features({"tag"}).learn_map(train) << "\n";

  // "magenta" was never seen -- a stateless hash handles it for free (no error),
  // it simply lands in whichever bucket it hashes to.
  std::cout << "map(later) -- unseen 'magenta' still encodes:\n"
            << encoder.map(later) << "\n";

  return 0;
}
