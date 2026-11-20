// Read a column of product reviews: tokenise, drop filler words, count what people talk about and score how they feel.
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto reviews = pluss::make_note({"Great battery life, and the screen is gorgeous!",
                                   "The battery died after two weeks. Terrible quality.",
                                   "Screen is bright but the speakers are tinny.",
                                   "Love it. Battery lasts all day and charging is fast."});
  show("reviews", reviews);

  // 1. Lower-case, drop stop words (the, and, is, ...), then tokenise. A token batch is ragged: one row of words per review.
  auto batch = reviews.lower().nlp().without_stopwords();
  std::cout << "reviews " << batch.offsets().numel() - 1 << ", words kept " << batch.tokens().numel() << "\n";
  const auto offsets = batch.offsets().host();
  std::cout << "words per review: ";
  for (std::size_t r = 0; r + 1 < offsets.int_vector().size(); ++r) std::cout << offsets.int_vector()[r + 1] - offsets.int_vector()[r] << " ";
  std::cout << "\n";

  // 2. Count words across all reviews: the flat token Note holds every word once per occurrence.
  const auto words = batch.tokens().host();
  std::map<std::string, int> count;
  for (const auto &w : words.vector()) ++count[w];
  std::vector<std::pair<std::string, int>> ranked(count.begin(), count.end());
  std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) { return a.second > b.second || (a.second == b.second && a.first < b.first); });
  std::cout << "most mentioned: ";
  for (std::size_t i = 0; i < 4; ++i) std::cout << ranked[i].first << " (" << ranked[i].second << ")  ";
  std::cout << "\n";

  // 3. Which reviews mention the battery? contains() is a vectorised test; a Bool Note exposes its lanes as a span.
  auto mentions = reviews.lower().contains("battery");
  int count_battery = 0;
  for (const auto flag : mentions.bools()) count_battery += flag ? 1 : 0;
  std::cout << count_battery << " of " << reviews.numel() << " reviews mention the battery\n";
}
