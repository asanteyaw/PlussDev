<!-- generated from comparing_and_matching.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Comparing and matching text

[Back to Note by example](../note.md)

Two versions of a file, two listings of the same product: the question is how alike two texts are. This page diffs two configuration versions and finds near-duplicate product listings, with a threshold you can see and change.

## The mental model

Texts are compared at three levels: **lines** (what was added or removed), **words** (what changed inside a line) and **vocabulary** (how much of the word set is shared, the Jaccard similarity: shared words over all words, from 0 to 1). The first two come back as a ragged token batch of marked differences, the third as a number per pair.

## What changed in a file

Two versions of a configuration file. The program lists the differing lines, measures the similarity of the vocabularies, looks inside one changed line and counts lines.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto before = pluss::make_note("max_connections = 100\ntimeout_seconds = 30\nlog_level = info\nretry = true\n");
  auto after = pluss::make_note("max_connections = 250\ntimeout_seconds = 30\nlog_level = debug\nretry = true\ncache = enabled\n");

  // 1. Line level: which lines were removed or added? diff_lines returns the differing lines of the pair.
  auto changed = before.diff_lines(after);
  show("differing lines", changed.tokens());

  // 2. A single number for "how alike": the Jaccard similarity of the word sets (1 = same vocabulary, 0 = nothing shared).
  auto similarity = before.nlp().jaccard_tokens(after);
  show("word-set similarity", similarity);

  // 3. Inside one changed line, which words moved? Compare the two versions of the log_level line.
  auto old_line = pluss::make_note("log_level = info");
  auto new_line = pluss::make_note("log_level = debug");
  show("word diff of the log_level line", old_line.nlp().diff_words(new_line).tokens());

  // 4. A review summary: count the lines in each version and the difference.
  const auto lines_before = before.split_lines().tokens().numel();
  const auto lines_after = after.split_lines().tokens().numel();
  std::cout << "lines before " << lines_before << ", after " << lines_after << ", net " << lines_after - lines_before << "\n";
}
```

Output

```text
differing lines: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["-max_connections = 100", "+max_connections = 250", "-log_level = info", "+log_level = debug", "+cache = enabled"]
word-set similarity: Note(shape=[], dim=0, numel=1, dtype=Float64)
0.5
word diff of the log_level line: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["-info", "+debug"]
lines before 4, after 5, net 1
```
`diff_lines` marks removed lines with `-` and added lines with `+`: the connection limit changed, the log level changed, and one setting was added. The vocabulary similarity is `0.5`, which says the files share half of their words, a blunt number that does not say where. The word diff of the `log_level` line is the precise tool: `-info`, `+debug`. The line counts (four before, five after) close the review summary.

## Near-duplicate listings

Six product listings, three products, written differently. The program normalises them, computes the Jaccard similarity of every pair and groups the listings at two thresholds.

```cpp
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
```

Output

```text
listings: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["Apple iPhone 15 Pro, 256GB - Blue Titanium", "iPhone 15 Pro 256 GB blue titanium (Apple)", "Samsung Galaxy S24 Ultra 512GB Black", "Galaxy S24 Ultra by Samsung, 512 GB, black", "Apple AirPods Pro 2nd generation", "apple iphone 15 pro 256gb blue titanium"]
clean: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["apple iphone 15 pro 256gb blue titanium", "iphone 15 pro 256 gb blue titanium apple", "samsung galaxy s24 ultra 512gb black", "galaxy s24 ultra by samsung 512 gb black", "apple airpods pro 2nd generation", "apple iphone 15 pro 256gb blue titanium"]
similarity of listing 0 with each listing: 1 0.666667 0 0 0.2 1
threshold 0.6: 4 groups, listing -> group: 0 0 1 2 3 0
threshold 0.5: 3 groups, listing -> group: 0 0 1 1 2 0
similarity of the two Samsung listings: 0.555556
```
Normalising (lower case, no punctuation, single spaces) makes the first and last listings identical (similarity `1`) and the reordered second listing `0.667`; the AirPods listing shares only `0.2` with them, so it stays apart. The two Samsung listings are the interesting case: their similarity is `0.556` because one writes `512GB` and the other `512 GB, by`, and at a threshold of `0.6` they are not grouped, giving four groups, while at `0.5` they are, giving the three real products. The threshold is a decision with a cost, and the output shows both sides of it. Normalising unit spellings (`512GB` against `512 GB`) before comparing would raise the similarity without changing the threshold.

## Decisions

- **Which level:** lines for "what changed", words for "where in the line", Jaccard for "are these the same thing".
- **Normalise first:** the similarity of un-normalised text mostly measures formatting.
- **Thresholds:** print the similarities, look at the pairs near the threshold, and choose; do not trust a default.
- **Scale:** comparing every pair is quadratic; for thousands of listings, bucket by a key (brand, model number) first.

## Where next

Reference pages: [`diff_lines`](../../sphinx/api/note/diff_lines.md), [`split_lines`](../../sphinx/api/note/split_lines.md), [`lower`](../../sphinx/api/note/lower.md), [`collapse_spaces`](../../sphinx/api/note/collapse_spaces.md), [`remove_punctuation`](../../sphinx/api/note/remove_punctuation.md), [`jaccard_tokens`](../../sphinx/api/note-nlp/jaccard_tokens.md), [`diff_words`](../../sphinx/api/note-nlp/diff_words.md). Next: [Documents: generating and checking](documents.md).
