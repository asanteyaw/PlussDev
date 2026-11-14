<!-- generated from parsing_and_counting.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Parsing, counting and reading text

[Back to Note by example](../note.md)

Logs and reviews hold facts inside sentences. This page pulls fields out of a server log, selects the failing requests, finds the slowest one, and then tokenises product reviews to count what people talk about.

## The mental model

Parsing is replacement with intent: a regular expression with a capture group, replaced by `$1`, keeps only the part you want and discards the rest, for every cell at once. Tokenising is different: each cell becomes a *variable number* of words, which a rectangular `Note` cannot hold, so the result is a **token batch**, a flat `Note` of all the words plus an `offsets` array that says where each cell's words start and end. When you need to leave Pluss, say so: `host()` copies the cells into ordinary C++ values.

## Triaging a server log

Six request lines with a time, a method, a path, a status code and a latency. The program isolates the status, selects the server errors, lists the distinct paths and finds the slowest request.

```cpp
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto lines = pluss::make_note({"2026-11-13T09:00:01Z GET /api/users 200 12ms",
                                 "2026-11-13T09:00:02Z GET /api/orders 500 340ms",
                                 "2026-11-13T09:00:02Z POST /api/orders 201 45ms",
                                 "2026-11-13T09:00:04Z GET /api/users/42 404 8ms",
                                 "2026-11-13T09:00:05Z GET /api/orders 503 1200ms",
                                 "2026-11-13T09:00:07Z GET /health 200 1ms"});
  std::cout << lines.numel() << " log lines\n";

  // 1. The status code is the three-digit number between spaces; replace everything around it to isolate it.
  auto status = lines.regex().replace(R"(^.* (\d{3}) \d+ms$)", "$1");
  show("status", status);

  // 2. Server errors are the codes starting with 5. starts_with is a vectorised test over the whole column.
  auto server_error = status.starts_with("5");
  std::vector<std::int64_t> failing_rows;   // a Bool Note exposes its lanes as a span; collect the rows that are true
  std::int64_t row = 0;
  for (const auto flag : server_error.bools()) {
    if (flag) failing_rows.push_back(row);
    ++row;
  }
  show("lines with a 5xx status", lines.take(failing_rows));

  // 3. Which paths are involved? Extract the token that starts with a slash.
  auto path = lines.regex().replace(R"(^\S+ \S+ (/\S*) .*$)", "$1");
  show("paths", path);
  show("distinct paths", path.unique());

  // 4. Latency: keep the digits before "ms" and convert the column to numbers.
  auto millis = lines.regex().replace(R"(^.* (\d+)ms$)", "$1");
  show("latency text", millis);
  double slowest = 0.0;
  const auto cells = millis.host();   // keep the owning copy alive while you read from it
  for (const auto &text : cells.vector()) slowest = std::max(slowest, std::stod(text));
  std::cout << "slowest request: " << slowest << " ms\n";
}
```

Output

```text
6 log lines
status: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["200", "500", "201", "404", "503", "200"]
lines with a 5xx status: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2026-11-13T09:00:02Z GET /api/orders 500 340ms", "2026-11-13T09:00:05Z GET /api/orders 503 1200ms"]
paths: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["/api/users", "/api/orders", "/api/orders", "/api/users/42", "/api/orders", "/health"]
distinct paths: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["/api/orders", "/api/users", "/api/users/42", "/health"]
latency text: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["12", "340", "45", "8", "1200", "1"]
slowest request: 1200 ms
```
The pattern `^.* (\d{3}) \d+ms$` matches the whole line and captures the three-digit status; replacing the line by `$1` leaves just the status for each of the six lines. `starts_with("5")` flags the codes beginning with 5, and `take` selects the two failing lines (500 and 503, both on `/api/orders`), which immediately says where to look. The same technique extracts the paths, and `unique` reduces six to four distinct paths. The latency is extracted as text; the program converts it with `std::stod` on the host copy and finds `1200 ms`. Two details of that last step matter. Text is not parsed into numbers by Pluss, because a `Note` is closed over its type, so the conversion is yours to write; and the owning copy returned by `host()` must be kept in a variable while you read from it, as the program does.

## Reading reviews

Four reviews of a gadget. The program lower-cases them, drops stop words and tokenises, counts the words across all reviews, and checks how many mention the battery.

```cpp
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
```

Output

```text
reviews: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Great battery life, and the screen is gorgeous!", "The battery died after two weeks. Terrible quality.", "Screen is bright but the speakers are tinny.", "Love it. Battery lasts all day and charging is fast."]
reviews 4, words kept 24
words per review: 5 7 5 7 
most mentioned: battery (3)  screen (2)  after (1)  all (1)  
3 of 4 reviews mention the battery
```
The token batch holds `24` words in `4` rows, of `5`, `7`, `5` and `7` words, and the offsets recover that structure exactly. Counting is ordinary C++ over the flat token list: `battery` is the most mentioned word (three reviews), then `screen` (two). The last line asks the same question the other way round: `contains` flags the reviews that mention the battery, and three of four do. A list of the most common words is the quickest way to find what to look at next, but it is only a start: it cannot tell you whether the mentions are good or bad.

## A Markdown document: outline and links

A short Markdown file with headings and links. The program lists the headings as an indented contents list and then collects the link targets and sorts them into external and local.

```cpp
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
```

Output

```text
lines in the document: 8
headings: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["# Installing", "## Requirements", "## Troubleshooting", "### Linker errors"]
- Installing
  - Requirements
  - Troubleshooting
    - Linker errors
link targets: Note(shape=[6], dim=1, numel=6, encoding=utf8)
["quickstart.md", "https://example.com/ref", "toolchains.md", "faq.md", "http://example.com/old", "linking.md"]
external 2, local 4
distinct local files to check: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["faq.md", "linking.md", "quickstart.md", "toolchains.md"]
```
`split_lines` turns the document into eight lines, and `starts_with("#")` flags four headings; the number of leading hashes is the level, which the program turns into the indentation. For links, the pattern `\[[^\]]*\]\(([^)]+)\)` has a capture group, and `findall` returns what the group captured, the targets, rather than the whole match. Two of the six targets are external; the four local ones name three distinct files (`linking.md` is linked twice), which is the list to check against the file system. A pattern that matches the whole link would have returned `[text](target)` and needed another replace to peel it.

## Decisions

- **Capture groups for extraction:** `regex().replace(pattern, "$1")` with the whole line in the pattern is the shortest way to pull one field from every line.
- **Fields to numbers:** extract as text, convert on the host, and decide there what a failed conversion means.
- **Ragged results:** expect a token batch from tokenisers and keep both the tokens and the offsets.
- **Counting words:** tokenise, drop stop words, count on the host; use `unique` when you only need the vocabulary.

## Where next

Reference pages: [`replace`](../../sphinx/api/note-regex/replace.md), [`findall`](../../sphinx/api/note-regex/findall.md), [`starts_with`](../../sphinx/api/note/starts_with.md), [`take`](../../sphinx/api/note/take.md), [`unique`](../../sphinx/api/note/unique.md), [`contains`](../../sphinx/api/note/contains.md), [`host`](../../sphinx/api/note/host.md), [`lower`](../../sphinx/api/note/lower.md), [`split_lines`](../../sphinx/api/note/split_lines.md), [`without_stopwords`](../../sphinx/api/note-nlp/without_stopwords.md), [`tokens`](../../sphinx/api/note-nlp/tokens.md), and the [TokenBatch reference](../../sphinx/tokenbatch_api_reference.md). Next: [Comparing and matching text](comparing_and_matching.md).
