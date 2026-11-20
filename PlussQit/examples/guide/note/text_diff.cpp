// What changed between two versions of a document? List the lines that differ, measure how different the texts are, and
// highlight the words that changed inside a line.
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
