// Pull a table of contents and the link list out of a Markdown document, then check the links.
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto document = pluss::make_note(
      "# Installing\n"
      "See [the quickstart](quickstart.md) first, then the [reference](https://example.com/ref).\n"
      "## Requirements\n"
      "Needs a C++20 compiler. Details in [toolchains](toolchains.md).\n"
      "## Troubleshooting\n"
      "Check [the FAQ](faq.md) and [the old guide](http://example.com/old).\n"
      "### Linker errors\n"
      "Read `linking.md` for [more](linking.md).\n");
  auto lines = document.split_lines();
  std::cout << "lines in the document: " << lines.tokens().numel() << "\n";

  // 1. Headings: the lines that start with '#'. The number of hashes is the level.
  auto heading = lines.tokens().starts_with("#");
  std::vector<std::int64_t> heading_rows;
  std::int64_t row = 0;
  for (const auto flag : heading.bools()) {
    if (flag) heading_rows.push_back(row);
    ++row;
  }
  auto headings = lines.tokens().take(heading_rows);
  show("headings", headings);

  // 2. An indented contents list: two spaces per level below the first.
  const auto cells = headings.host();
  for (const auto &h : cells.vector()) {
    std::size_t level = 0;
    while (level < h.size() && h[level] == '#') ++level;
    std::cout << std::string(2 * (level - 1), ' ') << "- " << h.substr(level + 1) << "\n";
  }

  // 3. Links: everything of the form [text](target). With a capture group, findall returns the captured part, the target.
  auto links = document.regex().findall(R"(\[[^\]]*\]\(([^)]+)\))");
  auto targets = links.tokens();
  show("link targets", targets);

  // 4. Classify: external links start with http, the rest are files that must exist next to the document.
  auto external = targets.starts_with("http");
  std::vector<std::int64_t> local_rows;
  row = 0;
  for (const auto flag : external.bools()) {
    if (!flag) local_rows.push_back(row);
    ++row;
  }
  std::cout << "external " << targets.numel() - static_cast<std::int64_t>(local_rows.size()) << ", local " << local_rows.size() << "\n";
  show("distinct local files to check", targets.take(local_rows).unique());
}
