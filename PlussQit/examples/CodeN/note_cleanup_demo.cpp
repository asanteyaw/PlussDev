#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "pluss/coden.h"

int main() {
  const std::array<std::int64_t, 3> sizes{2, 2, 1};
  pluss::Note n1 = pluss::make_note(
      std::vector<std::string>{"Row0 https://a.b", "Row1 dev@example.com",
                               "<b>Row2</b>", "- Row3"},
      std::span<const std::int64_t>(sizes.data(), sizes.size()));

  pluss::Note cleaned = n1.nlp_remove_urls()
                          .nlp_remove_emails()
                          .nlp_remove_html()
                          .nlp_remove_bullets();

  std::cout << "dim=" << cleaned.dim() << " numel=" << cleaned.numel() << "\n";
  for (std::int64_t i = 0; i < cleaned.numel(); ++i) {
    std::cout << i << ": " << cleaned.at(i) << "\n";
  }
  return 0;
}
