// Find listings that are the same product written differently: normalise the text, compare the word sets of every pair
// and group the pairs that are alike enough.
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto listing = pluss::make_note({"Apple iPhone 15 Pro, 256GB - Blue Titanium",
                                   "iPhone 15 Pro 256 GB blue titanium (Apple)",
                                   "Samsung Galaxy S24 Ultra 512GB Black",
                                   "Galaxy S24 Ultra by Samsung, 512 GB, black",
                                   "Apple AirPods Pro 2nd generation",
                                   "apple iphone 15 pro 256gb blue titanium"});
  show("listings", listing);

  // 1. Normalise: lower case, punctuation off, runs of spaces to one.
  auto clean = listing.lower().remove_punctuation().collapse_spaces();
  show("clean", clean);

  // 2. Compare every listing with every other by the Jaccard similarity of their word sets: shared words over all words.
  const std::int64_t n = clean.numel();
  const auto cells = clean.host();
  std::vector<std::vector<double>> similarity(n, std::vector<double>(n, 0.0));
  for (std::int64_t i = 0; i < n; ++i)
    for (std::int64_t j = 0; j < n; ++j) {
      auto score = pluss::make_note(cells.vector()[i]).nlp().jaccard_tokens(pluss::make_note(cells.vector()[j]));
      similarity[i][j] = score.host().float_scalar();
    }
  std::cout << "similarity of listing 0 with each listing:";
  for (std::int64_t j = 0; j < n; ++j) std::cout << " " << similarity[0][j];
  std::cout << "\n";

  // 3. Group: a listing joins the first earlier listing it resembles by at least the threshold, otherwise it starts a new group.
  //    The threshold is a decision, so look at two of them.
  for (const double threshold : {0.6, 0.5}) {
    std::vector<int> group(n, -1);
    int groups = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      for (std::int64_t j = 0; j < i && group[i] < 0; ++j)
        if (similarity[i][j] >= threshold) group[i] = group[j];
      if (group[i] < 0) group[i] = groups++;
    }
    std::cout << "threshold " << threshold << ": " << groups << " groups, listing -> group:";
    for (std::int64_t i = 0; i < n; ++i) std::cout << " " << group[i];
    std::cout << "\n";
  }
  std::cout << "similarity of the two Samsung listings: " << similarity[2][3] << "\n";
}
