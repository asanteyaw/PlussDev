# Note Guide (Usage-First)

## Includes

```cpp
#include <pluss/pluss.h>
// or
#include <pluss/coden/coden.h>
#include <pluss/coden/note_factory.h>
```

## How to read this page

- Snippets are practical, runnable use cases.
- Comments explain why a method is useful.
- Full current call-pattern coverage is included at the end.

## Setup Once, Reuse Everywhere

```cpp
auto note = pluss::make_note("One. Two!  Email: bob@example.com");
auto docs = pluss::make_note({
  "Line A\nLine B",
  "A,b;c and more text"
});
```

## Construction: Real Ways You Will Use

```cpp
// Scalar / 1D / ND UTF-8
auto n0 = pluss::make_note("hello");
auto n1 = pluss::make_note({"alpha", "beta", "gamma"});
auto n2 = pluss::make_note({{"r1c1", "r1c2"}, {"r2c1", "r2c2"}});

// Flat data + shape
std::vector<std::string> flat{"a","b","c","d"};
auto n3 = pluss::make_note(flat, std::array<std::int64_t,2>{2,2});

// Derived constructors
auto by_lines = pluss::note::from_lines("l1\nl2\nl3");
auto by_words = pluss::note::from_words("alpha beta gamma");

// Fill helpers
auto fulln = pluss::note::full({2,3}, "x");
auto emptyn = pluss::note::empty_like(fulln);
```

```cpp
// Bytes mode (raw bytes, not UTF-8 semantic text)
auto b0 = pluss::note::bytes("\x01\x02");
auto b1 = pluss::note::bytes({"\x01", "\x02", "\x03"});
auto b2 = pluss::note::bytes({{"\x0A", "\x0B"}, {"\x0C", "\x0D"}});
```

## Core Text Cleanup and Normalization

```cpp
// Typical cleaning chain for user input.
auto clean = note
  .lower()                   // normalize case
  .collapse_whitespace()     // shrink runs of spaces/newlines
  .strip();                  // trim edges

// Character-set / punctuation cleanup.
auto ascii = note.to_ascii();
auto no_punct = note.remove_punctuation();
auto alnum = note.keep_alnum();
```

Helpful note:

- Prefer small composable chains (`lower` + `collapse_whitespace` + `strip`) instead of one huge transform so behavior is explicit.

## Replace, Regex, Prefix/Suffix Shaping

```cpp
auto r1 = note.replace("Email", "Contact");
auto r2 = note.replace_first("One", "1");
auto r3 = note.remove_all("!");

auto rgx = note.regex_replace("\\b\\w+@\\w+\\.\\w+\\b", "<EMAIL>");

auto p1 = note.ensure_prefix("[DOC] ");
auto p2 = note.ensure_suffix(".");
auto q  = note.quote();
auto uq = q.unquote();
```

## Predicates and Lengths

```cpp
// Vector<uint8_t> outputs: one element per Note cell.
auto has_email = note.contains("@");
auto starts = note.starts_with("One");
auto ends = note.ends_with("com");

// Length metrics
auto bytes = note.lengths_bytes();
auto chars = note.lengths_chars();
auto graphemes = note.lengths_graphemes();
```

## Tokenization and TokenBatch Usage

```cpp
// TokenBatch gives tokens + offsets per input cell.
auto words = docs.tokenize_words();
auto sents = docs.split_sentences();
auto punct = docs.split_on_punct();

for (std::int64_t i = 0; i < words.tokens().numel(); ++i) {
  std::cout << words.tokens().at(i) << "\n";
}
```

Helpful note:

- When working on search/retrieval features, tokenize once and reuse tokens instead of repeatedly re-splitting strings.

## NLP Family: Cleaning, Masking, Extraction

```cpp
auto nlp0 = note
  .nlp_remove_urls()
  .nlp_remove_emails()
  .nlp_remove_html()
  .nlp_simplify_whitespace();

auto masked = note.nlp_mask_pii();
auto masked_nums = note.nlp_mask_numbers();

auto ents = note.nlp_named_entities();
auto keyw = note.nlp_keywords(10);
auto acr  = note.nlp_extract_acronyms();
```

## TeX Family: Convert, Clean, Extract

```cpp
auto tex = pluss::make_note(
  "\\begin{document} Text \\textbf{bold} and $x+y$. \\label{l1} \\ref{l1} \\end{document}");

// Convert TeX to text for downstream NLP.
auto plain = tex.tex_to_text();

// Remove citation/ref metadata noise.
auto stripped = tex
  .tex_remove_labels()
  .tex_remove_refs()
  .tex_remove_citations();

// Structured extraction
auto cmds = tex.tex_commands();
auto envs = tex.tex_environments();
auto eqs  = tex.tex_equations(64);
```

Helpful note:

- TeX preprocessing works best as a pipeline: normalize -> strip metadata -> extract tokens.

## Encoding and Escaping Utilities

```cpp
auto codec = pluss::make_note("Hi!");

auto b64 = codec.base64_encode().base64_decode();
auto hex = codec.hex_encode().hex_decode();
auto url = pluss::make_note("a b+c").url_encode().url_decode();

auto html = codec.html_escape().html_unescape();
auto json = codec.json_escape();
```

## Shape Ops on Note Tensors

```cpp
auto grid = pluss::make_note({{"a","b","c"}, {"d","e","f"}});

auto r = grid.reshape({3,2});
auto t = grid.transpose(0,1);
auto p = grid.permute({1,0});
auto s = grid.select(0,1);
auto n = grid.narrow(1,0,2);
auto f = grid.flatten();
auto c = grid.contiguous();
```

## Compare, Diff, Alignment, and Similarity

```cpp
auto d1 = pluss::make_note("alpha beta gamma");
auto d2 = pluss::make_note("alpha theta gamma delta");

auto diff_words = d1.diff_words(d2);
auto align_tokens = d1.align_tokens(d2);
auto inter_tokens = d1.intersect_tokens(d2);

auto jac = d1.jaccard_tokens(d2);  // similarity score note
auto ov  = d1.overlap_tokens(d2);  // overlap score note
```

## Ordering and Deduping

```cpp
auto arr = pluss::make_note({"b", "a", "b"});
auto sorted = arr.sort();
auto uniq = sorted.unique();
auto dedup = arr.deduplicate();
```
