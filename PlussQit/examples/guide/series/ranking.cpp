// Rank candidates by score: order them, pick the best few, find each one's rank and see how scores are distributed.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto score = pluss::make_series({71.5, 88.0, 64.2, 92.3, 79.9, 88.0, 55.1, 83.4});
  show("scores", score);

  // 1. argsort gives the positions that would sort the scores; descending puts the best first.
  auto order = score.argsort(0, true);
  show("candidates best first", order);

  // 2. topk returns the k largest values together with where they sit.
  auto [best, where] = score.topk_pair(3);
  show("top three scores", best);
  show("top three positions", where);

  // 3. The rank of each candidate (0 = best) is the argsort of the order: it is the inverse permutation.
  auto rank = order.argsort(0);
  show("rank of each candidate", rank);

  // 4. Which candidates are above average, and how far?
  const double mean = score.mean().item<double>();
  auto above = score.gt(mean);
  std::cout << "mean score " << mean << "; above average: " << above.sum().item<double>() << " of " << score.numel() << "\n";
  show("distance from the mean", score - mean);

  // 5. A histogram of the scores in four equal bins between the lowest and highest score.
  show("histogram", score.histc(4));
}
