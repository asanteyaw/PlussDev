<!-- generated from cleaning_text.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Cleaning text and protecting data

[Back to Note by example](../note.md)

Text columns are messy in predictable ways: stray spaces, inconsistent capitals, invalid addresses, duplicates, and personal data that must not leave the building. This page cleans a customer table and prepares support tickets for sharing, with complete programs and printed results.

## The mental model

A `Note` is to text what a `Series` is to numbers: a block of cells with a shape, processed as a whole. Every operation applies to every cell and returns a `Note`, so a cleaning pipeline is a chain of calls with no loop. Two properties are worth knowing before the first program.

- **The type is closed.** A test such as `contains` or `regex().match` returns a *Bool* `Note`, not a `Series` and not a `std::vector<bool>`. You read its lanes with `bools()`, which borrows them from the `Note`, so keep the `Note` in a variable while you read.
- **Specialised work lives in accessors.** `note.regex()` holds the pattern operations and `note.nlp()` the text-analysis ones; the plain `Note` methods cover trimming, case, splitting and replacing.

## A customer table

Five customer rows with ragged spacing and capitals, and five email addresses of which one is malformed. The program tidies the names, validates the addresses with a regular expression, finds the duplicates and pulls out the domains.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto names = pluss::make_note({"  ada   LOVELACE ", "Grace Hopper", "alan turing", "GRACE  hopper", "Edsger  Dijkstra  "});
  auto emails = pluss::make_note({"Ada@Example.com", "grace.hopper@navy.mil", "alan@@bad", "GRACE.HOPPER@navy.mil", "ed@dijkstra.nl"});
  show("raw names", names);

  // 1. Names: trim the ends, collapse runs of spaces to one, and capitalise each word.
  auto clean_names = names.strip().collapse_spaces().title();
  show("clean names", clean_names);

  // 2. Emails: lower-case them, then test each whole address against a name@domain.tld pattern.
  auto lowered = emails.lower();
  auto valid = lowered.regex().match(R"([a-z0-9._-]+@[a-z0-9-]+(\.[a-z0-9-]+)+)");
  show("lower-cased", lowered);
  show("valid address?", valid);
  int invalid = 0;
  for (const auto ok : valid.bools()) invalid += ok ? 0 : 1;   // a Bool Note exposes its lanes as a span
  std::cout << "invalid addresses: " << invalid << "\n";

  // 3. Duplicates: two rows are the same customer when their cleaned names agree.
  auto unique_names = clean_names.unique();
  show("distinct customers", unique_names);
  std::cout << "records " << clean_names.numel() << ", distinct " << unique_names.numel() << "\n";

  // 4. A domain report: split each address at '@' and look at what follows.
  auto domain = lowered.regex().replace(R"(^.*@)", "");
  show("domains", domain);
}
```

Output

```text
raw names: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["  ada   LOVELACE ", "Grace Hopper", "alan turing", "GRACE  hopper", "Edsger  Dijkstra  "]
clean names: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["Ada Lovelace", "Grace Hopper", "Alan Turing", "Grace Hopper", "Edsger Dijkstra"]
lower-cased: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["ada@example.com", "grace.hopper@navy.mil", "alan@@bad", "grace.hopper@navy.mil", "ed@dijkstra.nl"]
valid address?: Note(shape=[5], dim=1, numel=5, dtype=Bool)
[true, true, false, true, true]
invalid addresses: 1
distinct customers: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Ada Lovelace", "Alan Turing", "Edsger Dijkstra", "Grace Hopper"]
records 5, distinct 4
domains: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["example.com", "navy.mil", "bad", "navy.mil", "dijkstra.nl"]
```
`strip`, `collapse_spaces` and `title` turn `"  ada   LOVELACE "` into `"Ada Lovelace"` for every row in three calls, and once the names are tidy the two Grace Hopper rows are visibly the same customer: `unique` returns four distinct customers from five rows, sorted. `regex().match` anchors the pattern to the whole cell, so it is a validity test; it flags `alan@@bad` and passes the rest. The domain column shows a limit of the replace: for the invalid address the pattern strips everything up to the last `@` and leaves `bad`. Compute domains only for rows the validity test passed.

## Preparing tickets for sharing

Three support tickets contain an HTML tag, an email address, a phone number, an order number, a URL, an amount, a date and typographic quotes. The program removes the markup, masks each kind of personal data with a placeholder, normalises the quotes and dashes, and finally audits the result.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto tickets = pluss::make_note({"<p>Hi, I'm Dana (dana.k@example.org). My order 88213 never arrived.</p>",
                                   "Call me on 555-0142 or see https://shop.example.com/orders/88213 for the tracking.",
                                   "Refund of $49.99 requested on 2026-11-02 – “still waiting”."});
  show("original", tickets);

  // 1. Remove markup, then mask the personal data. Each masker replaces what it finds with a placeholder.
  auto text = tickets.nlp().remove_html();
  auto masked = text.nlp().mask_emails().nlp().mask_urls().nlp().mask_dates().nlp().mask_currency().nlp().mask_numbers();
  show("masked", masked);

  // 2. Normalise typography so that downstream matching is not defeated by curly quotes and dashes.
  auto tidy = masked.normalize_quotes().normalize_dashes();
  show("normalised", tidy);

  // 3. Audit: no address, link or digit may remain. An audit only proves what it tests, so test for each kind of leak you care about.
  // The Bool Notes must stay alive while their span is read: bools() borrows from them.
  const auto has_email = tidy.contains("@"), has_url = tidy.contains("http"), has_digit = tidy.contains_any_digit();
  int leaks = 0;
  for (const auto f : has_email.bools()) leaks += f ? 1 : 0;
  for (const auto f : has_url.bools()) leaks += f ? 1 : 0;
  for (const auto f : has_digit.bools()) leaks += f ? 1 : 0;
  std::cout << "leaks found: " << leaks << "\n";
  const auto has_name = tidy.contains("Dana");
  std::cout << "names are not masked by these maskers: ticket 0 still names Dana: " << (has_name.bools()[0] ? "yes" : "no") << "\n";
}
```

Output

```text
original: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["<p>Hi, I'm Dana (dana.k@example.org). My order 88213 never arrived.</p>", "Call me on 555-0142 or see https://shop.example.com/orders/88213 for the tracking.", "Refund of $49.99 requested on 2026-11-02 – “still waiting”."]
masked: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Hi, I'm Dana ([EMAIL]). My order [NUM] never arrived.", "Call me on [NUM] or see [URL] for the tracking.", "Refund of [CUR] requested on [DATE] – “still waiting”."]
normalised: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Hi, I'm Dana ([EMAIL]). My order [NUM] never arrived.", "Call me on [NUM] or see [URL] for the tracking.", "Refund of [CUR] requested on [DATE] - \"still waiting\"."]
leaks found: 0
names are not masked by these maskers: ticket 0 still names Dana: yes
```
Each masker replaces what it finds with a placeholder (`[EMAIL]`, `[URL]`, `[DATE]`, `[CUR]`, `[NUM]`), so the structure of the sentence survives and the data does not. `normalize_quotes` and `normalize_dashes` replace the curly quotes and the en dash with plain ones, so that later matching is not defeated by typography. The audit tests for at-signs, links and digits and finds none. Read its last line carefully: the maskers do not know names, and the first ticket still names Dana. An audit proves only what it tests, so decide what counts as personal data first, mask that, and test for each kind.

## Text scraped from the web

Web text arrives wrapped in layers: percent-escapes, HTML entities, invisible characters and two ways of writing the same accented letter. The program unwraps them in order, repairs the invisible and composed forms, and round-trips a value through two text-safe encodings.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto scraped = pluss::make_note({"Fish &amp; Chips &#8212; &#163;5 &copy;", "caf%C3%A9%20au%20lait", "naïve​ word", "Café", "plain text"});
  show("scraped", scraped);

  // 1. Undo the escaping layers in order: URL percent-escapes, then HTML entities. html_unescape knows the five markup entities and
  //    numeric references (&#8212;); a named entity such as &copy; is left exactly as written, never dropped.
  auto decoded = scraped.url_decode().html_unescape();
  show("decoded", decoded);

  // 2. Remove zero-width characters: they are invisible, and they break equality, sorting and search.
  auto visible = decoded.strip_zero_width();
  show("without zero-width characters", visible);

  // 3. "Cafe" followed by a combining accent and "Café" with one precomposed character look the same but are different bytes.
  auto composed = visible.to_nfc();
  auto lengths_before = visible.lengths_chars();
  auto lengths_after = composed.lengths_chars();
  show("length in characters before NFC", lengths_before);
  show("length in characters after NFC ", lengths_after);

  // 4. Equality after normalisation: these two now match.
  auto a = pluss::make_note("Café").to_nfc();
  auto b = pluss::make_note("Café").to_nfc();
  const auto same = a.regex().match("Café");
  std::cout << "composed 'Cafe + accent' equals 'Café' after NFC: " << (same.bools()[0] ? "yes" : "no") << " (" << pluss::note::print(b.lengths_chars()) << ")\n";

  // 5. Round trip a value through a text-safe encoding, as when storing bytes in a text column.
  auto secret = pluss::make_note("report-2026-11-13");
  auto encoded = secret.base64_encode();
  show("base64", encoded);
  show("hex   ", secret.hex_encode());
  const auto back = encoded.base64_decode().host();
  std::cout << "decoded again: " << back.scalar() << "\n";
}
```

Output

```text
scraped: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["Fish &amp; Chips &#8212; &#163;5 &copy;", "caf%C3%A9%20au%20lait", "naïve​ word", "Café", "plain text"]
decoded: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["Fish & Chips — £5 &copy;", "café au lait", "naïve​ word", "Café", "plain text"]
without zero-width characters: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["Fish & Chips — £5 &copy;", "café au lait", "naïve word", "Café", "plain text"]
length in characters before NFC: Note(shape=[5], dim=1, numel=5, dtype=Int64)
[24, 12, 10, 5, 10]
length in characters after NFC : Note(shape=[5], dim=1, numel=5, dtype=Int64)
[24, 12, 10, 4, 10]
composed 'Cafe + accent' equals 'Café' after NFC: yes (Note(shape=[], dim=0, numel=1, dtype=Int64)
4)
base64: Note(shape=[], dim=0, numel=1, encoding=utf8)
"cmVwb3J0LTIwMjYtMTEtMTM="
hex   : Note(shape=[], dim=0, numel=1, encoding=utf8)
"7265706f72742d323032362d31312d3133"
decoded again: report-2026-11-13
```
`url_decode` turns `caf%C3%A9%20au%20lait` into `café au lait`, and `html_unescape` turns `&amp;`, `&#8212;` and `&#163;` into `&`, `—` and `£`. It knows the five markup entities and numeric references only, so `&copy;` is left exactly as written: nothing is dropped, but nothing outside that set is converted either, and an audit for leftover `&` is how you notice. `strip_zero_width` removes the invisible character in `naïve word`, whose length was 10 characters with the invisible one counted. The last two cells look identical, `Café`, but the first is five characters (`e` plus a combining accent) and the second four; after `to_nfc` both are four and compare equal. `base64_encode` and `hex_encode` give text-safe forms of a value and `base64_decode` returns it exactly.

## Slugs and file names

Five article titles with accents, symbols and one pair that differ only in case and accents. The program makes URL slugs, finds the collision and resolves it, and builds file names.

```cpp
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
```

Output

```text
titles: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["Café Culture in São Paulo!", "  10 Tips & Tricks: C++ Edition  ", "Résumé — Writing Guide", "Café culture in sao paulo", "What is \"Big-O\"?"]
plain: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["cafe culture in sao paulo!", "  10 tips & tricks: c++ edition  ", "resume ? writing guide", "cafe culture in sao paulo", "what is \"big-o\"?"]
slug: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["cafe-culture-in-sao-paulo", "10-tips-tricks-c-edition", "resume-writing-guide", "cafe-culture-in-sao-paulo", "what-is-big-o"]
titles 5, distinct slugs 4
final slugs: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["cafe-culture-in-sao-paulo", "10-tips-tricks-c-edition", "resume-writing-guide", "cafe-culture-in-sao-paulo-2", "what-is-big-o"]
file names: Note(shape=[5], dim=1, numel=5, encoding=utf8)
["2026-11-13-cafe-culture-in-sao-paulo.md", "2026-11-13-10-tips-tricks-c-edition.md", "2026-11-13-resume-writing-guide.md", "2026-11-13-cafe-culture-in-sao-paulo-2.md", "2026-11-13-what-is-big-o.md"]
name lengths: Note(shape=[5], dim=1, numel=5, dtype=Int64)
[39, 38, 34, 41, 27]
```
`strip_accents`, `to_ascii` and `lower` give plain text, though `to_ascii` replaces the em dash with `?`, which the next step then turns into a separator along with every other run of non-alphanumerics. Two titles collapse to `cafe-culture-in-sao-paulo`, so five titles give only four distinct slugs; a uniqueness check is part of any slug pipeline. The program numbers the repeats (`-2`) on the host, because "the second occurrence of this value" depends on order, and then `ensure_prefix` and `ensure_suffix` build the file names, whose lengths are checked against a limit. Notice what a slug loses: `C++` becomes `c`, so decide whether symbols that carry meaning need spelling out before slugging.

## Decisions

- **Cleaning order:** strip and collapse whitespace first, change case second, validate last; each step makes the next one reliable.
- **Validation or repair:** use `regex().match` to *decide* whether a value is valid; do not repair a value you cannot trust.
- **Duplicates:** compare after cleaning, never before.
- **Masking:** mask before you analyse or share, audit after, and name what the maskers cannot do (names, free-form addresses).
- **Reading Bool results:** keep the Bool `Note` in a variable and read `bools()`; the span borrows from it.

## Where next

Reference pages: [`strip`](../../sphinx/api/note/strip.md), [`collapse_spaces`](../../sphinx/api/note/collapse_spaces.md), [`title`](../../sphinx/api/note/title.md), [`lower`](../../sphinx/api/note/lower.md), [`unique`](../../sphinx/api/note/unique.md), [`contains`](../../sphinx/api/note/contains.md), [`contains_any_digit`](../../sphinx/api/note/contains_any_digit.md), [`normalize_quotes`](../../sphinx/api/note/normalize_quotes.md), [`normalize_dashes`](../../sphinx/api/note/normalize_dashes.md), [`bools`](../../sphinx/api/note/bools.md), [`url_decode`](../../sphinx/api/note/url_decode.md), [`html_unescape`](../../sphinx/api/note/html_unescape.md), [`strip_zero_width`](../../sphinx/api/note/strip_zero_width.md), [`to_nfc`](../../sphinx/api/note/to_nfc.md), [`strip_accents`](../../sphinx/api/note/strip_accents.md), [`to_ascii`](../../sphinx/api/note/to_ascii.md), [`ensure_prefix`](../../sphinx/api/note/ensure_prefix.md), [`base64_encode`](../../sphinx/api/note/base64_encode.md), [`match`](../../sphinx/api/note-regex/match.md), [`replace`](../../sphinx/api/note-regex/replace.md), [`remove_html`](../../sphinx/api/note-nlp/remove_html.md), [`mask_emails`](../../sphinx/api/note-nlp/mask_emails.md), [`mask_urls`](../../sphinx/api/note-nlp/mask_urls.md), [`mask_dates`](../../sphinx/api/note-nlp/mask_dates.md), [`mask_currency`](../../sphinx/api/note-nlp/mask_currency.md), [`mask_numbers`](../../sphinx/api/note-nlp/mask_numbers.md). Next: [Parsing, counting and reading text](parsing_and_counting.md).
