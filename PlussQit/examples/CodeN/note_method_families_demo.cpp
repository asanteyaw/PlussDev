#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>

int main() {
  pluss::Note scalar = pluss::content("alpha beta beta");
  std::cout << "replace_first: " << scalar.replace_first("beta", "X").at(0) << "\n";
  std::cout << "remove_all:    " << scalar.remove_all("beta").at(0) << "\n";
  std::cout << "quoted:        " << scalar.quote().at(0) << "\n";

  const std::array<std::int64_t, 1> sizes{2};
  pluss::Note docs = pluss::content(
      std::vector<std::string>{"One.\nTwo!", "A,b;c"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  auto words = docs.tokenize_words();
  std::cout << "tokenize_words offsets:";
  for (std::int64_t off : words.offsets()) std::cout << " " << off;
  std::cout << "\n";
  for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
    std::cout << "  word[" << i << "] = " << words.tokens().at(i) << "\n";
  }

  pluss::Note tex = pluss::content(
      "\\documentclass{article}\n"
      "% comment\n"
      "\\begin{document}\n"
      "Text \\textbf{bold} and $x+y$.\n"
      "\\end{document}\n");
  std::cout << "tex_to_text:   " << tex.tex_to_text().at(0) << "\n";
  return 0;
}
