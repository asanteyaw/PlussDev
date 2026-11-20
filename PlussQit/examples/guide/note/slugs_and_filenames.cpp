// Turn article titles into safe URL slugs and file names: remove accents and symbols, normalise case and spacing, and make the results unique.
#include <pluss/pluss.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto titles = pluss::make_note({"Café Culture in São Paulo!", "  10 Tips & Tricks: C++ Edition  ", "Résumé — Writing Guide", "Café culture in sao paulo", "What is \"Big-O\"?"});
  show("titles", titles);

  // 1. Accents off, then to plain ASCII, lower case. strip_accents leaves the letter and drops the mark.
  auto plain = titles.strip_accents().to_ascii().lower();
  show("plain", plain);

  // 2. Keep letters and digits, turn every run of anything else into one hyphen, and trim hyphens at the ends.
  auto slug = plain.regex().replace(R"([^a-z0-9]+)", "-").regex().replace(R"(^-+|-+$)", "");
  show("slug", slug);

  // 3. Slugs must be unique. Two titles collapsed to the same slug: find them.
  auto distinct = slug.unique();
  std::cout << "titles " << slug.numel() << ", distinct slugs " << distinct.numel() << "\n";

  // 4. Make them unique by suffixing later duplicates with their occurrence number (-2, -3, ...).
  const auto cells = slug.host();
  std::map<std::string, int> seen;
  std::vector<std::string> unique_slugs;
  for (const auto &s : cells.vector()) unique_slugs.push_back(++seen[s] == 1 ? s : s + "-" + std::to_string(seen[s]));
  auto final_slugs = pluss::make_note(unique_slugs);
  show("final slugs", final_slugs);

  // 5. File names: prefix with a date and add an extension, then check the length limit.
  auto files = final_slugs.ensure_prefix("2026-11-13-").ensure_suffix(".md");
  show("file names", files);
  auto length = files.lengths_chars();
  show("name lengths", length);
}
