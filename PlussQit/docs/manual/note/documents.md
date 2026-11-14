<!-- generated from documents.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Documents: generating and checking

[Back to Note by example](../note.md)

Text is also something you produce and verify. This page personalises a message for every row of a table and then checks a LaTeX manuscript for the mistakes that cost a rejected submission: references to labels that do not exist, and citations with no bibliography entry.

## The mental model

Generating text is replacement with data: a template with placeholders, one value per row. Checking text is extraction plus set arithmetic: pull out what is used, pull out what is defined, and compare the two sets. The extraction side has accessors for structured formats; for LaTeX, `note.tex()` knows sections, labels, references and citations.

## A mail merge

Four customers, one missing a name. The program fills a template for each, falls back to a default for the empty name, checks that no placeholder survives, and joins the letters into one document.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto name = pluss::make_note({"Ada", "Grace", "Alan", ""});
  auto product = pluss::make_note({"Analytical Engine", "Compiler", "Bombe", "Spare part"});
  auto due = pluss::make_note({"2026-12-01", "2026-12-15", "2026-11-30", "2026-12-20"});
  auto template_text = pluss::make_note("Dear {name}, your {product} order is due on {due}.");

  // 1. Fill the template row by row: replace each placeholder by the matching value; an empty name falls back to "customer".
  const auto names = name.host();
  const auto products = product.host();
  const auto dues = due.host();
  std::vector<std::string> messages;
  for (std::size_t i = 0; i < names.vector().size(); ++i) {
    auto one = template_text.replace("{name}", names.vector()[i].empty() ? "customer" : names.vector()[i]).replace("{product}", products.vector()[i]).replace("{due}", dues.vector()[i]);
    messages.push_back(one.host().scalar());
  }
  auto letters = pluss::make_note(messages);
  show("letters", letters);

  // 2. Quality control: a letter must have no placeholder left and no doubled spaces.
  auto left_over = letters.contains("{");
  int bad = 0;
  for (const auto f : left_over.bools()) bad += f ? 1 : 0;
  std::cout << "letters with an unfilled placeholder: " << bad << "\n";
  show("empty names replaced by", letters.take({3}));

  // 3. One document from all letters: join with a separator and count the length.
  auto bundle = letters.join("\n---\n");
  std::cout << "bundle length " << bundle.lengths_chars().host().int_scalar() << " characters, " << letters.numel() << " letters\n";
}
```

Output

```text
letters: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Dear Ada, your Analytical Engine order is due on 2026-12-01.", "Dear Grace, your Compiler order is due on 2026-12-15.", "Dear Alan, your Bombe order is due on 2026-11-30.", "Dear customer, your Spare part order is due on 2026-12-20."]
letters with an unfilled placeholder: 0
empty names replaced by: Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Dear customer, your Spare part order is due on 2026-12-20."]
bundle length 235 characters, 4 letters
```
Each letter is the template with `{name}`, `{product}` and `{due}` replaced for its row, and the empty name becomes `customer` instead of leaving `Dear ,`. The quality-control check is the habit to copy: after generating, test for the placeholder syntax, here `{`, and expect zero. `join` produces one document of 235 characters from the four letters.

## A manuscript check

A short LaTeX source with three sections, four labels, a citation of two keys at once and a reference to a section that does not exist yet. The program lists the outline and then compares used against defined for both references and citations.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <set>
#include <string>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto source = pluss::make_note(
      "\\section{Introduction}\\label{sec:intro}\n"
      "Prior work \\cite{knuth84} and \\cite{lamport94,missing99} motivates this paper.\n"
      "\\section{Method}\\label{sec:method}\n"
      "As argued in Section~\\ref{sec:intro} and Section~\\ref{sec:results}, we proceed.\n"
      "\\begin{equation}\\label{eq:main} E = mc^2 \\end{equation}\n"
      "See Equation~\\ref{eq:main}.\n"
      "\\section{Results}\n");
  auto bibliography_keys = pluss::make_note({"knuth84", "lamport94", "unused2000"});

  // 1. The outline of the paper.
  show("sections", source.tex().sections().tokens());

  // 2. Every label defined and every reference used.
  auto labels = source.tex().labels().tokens();
  auto refs = source.tex().refs().tokens();
  show("labels defined", labels);
  show("labels referenced", refs);

  // 3. References to labels that are never defined: set difference on the host.
  const auto defined = labels.host();
  const auto used = refs.host();
  std::set<std::string> defined_set(defined.vector().begin(), defined.vector().end());
  for (const auto &r : used.vector())
    if (!defined_set.count(r)) std::cout << "undefined reference: " << r << "\n";

  // 4. Citations against the bibliography: cited but missing, and listed but never cited.
  // citations() returns one token per \cite command ("lamport94,missing99" is one); bibliography_keys() splits a command into its keys.
  show("per command", source.tex().citations().tokens());
  auto cited = source.tex().bibliography_keys().tokens();
  show("citation keys", cited);
  const auto cites = cited.host();
  const auto bib = bibliography_keys.host();
  std::set<std::string> bib_set(bib.vector().begin(), bib.vector().end()), cite_set(cites.vector().begin(), cites.vector().end());
  for (const auto &k : cite_set)
    if (!bib_set.count(k)) std::cout << "cited but not in the bibliography: " << k << "\n";
  for (const auto &k : bib_set)
    if (!cite_set.count(k)) std::cout << "in the bibliography but never cited: " << k << "\n";
}
```

Output

```text
sections: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Introduction", "Method", "Results"]
labels defined: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["sec:intro", "sec:method", "eq:main"]
labels referenced: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["sec:intro", "sec:results", "eq:main"]
undefined reference: sec:results
per command: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["knuth84", "lamport94,missing99"]
citation keys: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["knuth84", "lamport94", "missing99"]
cited but not in the bibliography: missing99
in the bibliography but never cited: unused2000
```
The outline is `Introduction`, `Method`, `Results`. The labels defined are `sec:intro`, `sec:method` and `eq:main`; the references used are `sec:intro`, `sec:results` and `eq:main`, so one reference, `sec:results`, points at a label that was never defined, because the Results section carries no `\label`. For citations, `citations()` returns one token per command: `lamport94,missing99` is a single token because it came from a single `\cite`. To compare keys you want them one by one, which is `bibliography_keys()`: with it the check finds that `missing99` is cited but not in the bibliography, and that `unused2000` is in the bibliography but never cited.

## Decisions

- **Defaults for missing data:** decide the fallback text before generating, so an empty cell does not produce a broken sentence.
- **Verify what you generate:** test for leftover placeholders and for doubled spaces.
- **Extract, then compare sets:** used minus defined is the list of broken references; defined minus used is clutter.
- **Right extractor:** `citations()` for commands, `bibliography_keys()` for keys.

## Where next

Reference pages: [`replace`](../../sphinx/api/note/replace.md), [`join`](../../sphinx/api/note/join.md), [`contains`](../../sphinx/api/note/contains.md), [`lengths_chars`](../../sphinx/api/note/lengths_chars.md), [`host`](../../sphinx/api/note/host.md), [`sections`](../../sphinx/api/note-tex/sections.md), [`labels`](../../sphinx/api/note-tex/labels.md), [`refs`](../../sphinx/api/note-tex/refs.md), [`citations`](../../sphinx/api/note-tex/citations.md), [`bibliography_keys`](../../sphinx/api/note-tex/bibliography_keys.md). Back to [Note by example](../note.md).
