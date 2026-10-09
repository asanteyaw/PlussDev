# `pluss::ai::nlp::MatcherStage`

`MatcherStage` finds every occurrence of a set of phrase rules in a document
and records each one as a labeled span in `Doc::matches`. It is the
deterministic counterpart to the learned stages: it has no training step, it
keeps no state beyond its patterns and options, and the same input always
produces the same matches. Use it when a phrase is exact enough to write down,
when matches must be explainable, or when a rule should be enforced regardless
of what a learned model would say.

The stage reads the token stream, so it runs after a tokenizer. It is most
useful after a lemmatizer, because patterns can then match any inflection of a
word. It does not produce tags, entities, or sentences. Its output is the match
list, and that list is a separate annotation from the entity list that
[`NerStage`](NerStage.md) fills.

Each call to `apply` clears the previous matches before scanning, so re-running
the stage on a document never accumulates duplicate spans.

## Signature

```cpp
pluss::ai::nlp::MatcherStage::MatcherStage(
    std::vector<pluss::ai::nlp::MatcherPattern> patterns = {},
    pluss::ai::nlp::MatcherOptions options = {});
pluss::ai::nlp::MatcherStage::MatcherStage(
    std::initializer_list<pluss::ai::nlp::MatcherPattern> patterns,
    pluss::ai::nlp::MatcherOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::matcher(
    std::vector<pluss::ai::nlp::MatcherPattern> patterns = {},
    pluss::ai::nlp::MatcherOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::matcher(
    std::initializer_list<pluss::ai::nlp::MatcherPattern> patterns,
    pluss::ai::nlp::MatcherOptions options = {});
```

## Construction

Use `stage::matcher` to place the stage in a workflow. Use the concrete class
when you want a typed handle, which you need for `add_pattern`.

Both constructors validate every pattern before the stage is returned: a
pattern with an empty label, an empty token list, or an empty token string
throws, and the message says which check failed. The initializer-list
form is for literal rule lists, and the vector form is for patterns that come
from a file or a loop.

A stage with no patterns is valid. It clears the matches and does nothing else,
which is useful as a placeholder while a rule set is being assembled.

## Options

The options struct is `pluss::ai::nlp::MatcherOptions`. It has two boolean
fields, and both default to `true`.

### `lowercase`

Controls whether comparison ignores case. When `true`, both the document token
and the pattern token are lowercased before they are compared, so `New`,
`new`, and `NEW` all match a pattern token `New`. When `false`, the comparison
is exact, and the case of each character matters.

Keep it on for most text. Turn it off for patterns where case carries meaning,
such as an acronym that a lowercase form would collide with, or a product name
whose capitalization distinguishes it from a common word.

Lowercasing is ASCII only. A non-ASCII letter keeps its case, so `Élise` and
`élise` do not match each other even when `lowercase` is on.

### `allow_overlap`

Controls whether two matches may cover the same token. When `true`, the scan
of each pattern advances one token after every match, so a pattern like
`{"a", "a"}` finds every pair in `a a a`, including the overlapping pairs at
positions 0 and 1. When `false`, the scan skips past each match, and a
candidate is rejected if it intersects any match already accepted.

The rejection applies across patterns, not just within one pattern. Patterns
are tried in the order they were given, so an earlier pattern wins a contested
span. A later pattern whose match would overlap an accepted one is dropped.
This matches how the entity stage treats its own sources when overlap is off,
and the Practical Examples show a case where the two patterns would otherwise
both survive.

Turn overlap on when every occurrence matters, for example when counting
repeated patterns. Turn it off when each token should belong to at most one
match, as when the matches feed a downstream stage that assumes disjoint spans.

## State

The stage keeps its patterns and its two options. It does not override `learn`,
and it has no learned state.

`dump_state_lines()` writes the options, then the pattern count, and then three
lines for each pattern: the label, the lemma flag, and the token list. Labels
and tokens are hex-encoded, so the saved form is stable for any text. A
pattern label of `P` appears as `50`. `load_state_lines()` decodes that
representation and rejects a file whose pattern count or fields do not match.

## Methods

### `name()`

Returns `"matcher"`.

### `add_pattern(pattern)`

Validates the pattern, appends it to the stage's list, and returns a reference
to the stage so calls can be chained. The new pattern is used the next time
`apply` runs. A pattern that fails validation throws, and the stage is left
unchanged.

The added pattern is tried after the patterns already in the list, so it can
never displace an earlier match when overlap is off.

### `apply(Doc&)`

Clears `Doc::matches`, then scans the tokens once per pattern. Each pattern
compares its tokens with every run of document tokens of the same length.
Each token is normalized the same way as the pattern's strings, using its lemma
when the pattern asks for lemmas and the lemma is non-empty, and its surface
form otherwise.

When a run matches and the overlap rule allows it, a `Span` with the pattern's
label is appended. When all patterns have been tried, the matches are
stable-sorted by start, then end, then label, so the output order does not
depend on pattern order.

### `dump_state_lines()`

Returns the options and every pattern in the hex-encoded form described under
State.

### `load_state_lines(lines)`

Replaces the options and the pattern list from a saved set of lines. It
validates the decoded patterns the same way the constructor does.

## Practical Examples

These examples show the stage on a realistic sentence, the overlap rule within
one pattern and across two, and a lemma-based pattern that keeps and loses
matches as the surface and lemma forms change.

### Example 1

Two patterns with different shapes, built with `add_pattern` after
construction. The stage finds every occurrence of each pattern and returns the
matches sorted by position, so the output reads in document order.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  auto matcher = std::make_shared<nlp::MatcherStage>();
  matcher->add_pattern({.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false})
      .add_pattern({.label = "TEAM", .tokens = {"Data", "Team"}, .use_lemmas = false});

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), matcher}};
  const nlp::Doc doc = wf.infer("The Data Team moved from New York to New York City.");

  std::cout << "matches = " << doc.matches.size() << "\n";
  for (const nlp::Span& match : doc.matches) {
    std::cout << match.label << ": \"" << match.text(doc) << "\" tokens ["
              << match.token_start << ", " << match.token_end << ")\n";
  }
  return 0;
}
```

Output

```text
matches = 3
TEAM: "Data Team" tokens [1, 3)
CITY: "New York" tokens [5, 7)
CITY: "New York" tokens [8, 10)
```

### Example 2

The overlap rule on three inputs. The first shows overlap within one pattern:
`a a a` contains two overlapping pairs, and both survive when overlap is on.
The second shows the rule across two patterns. `New York` and `York Times`
share the token `York`, and with overlap off only the earlier pattern keeps
its match.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool overlap : {true, false}) {
    nlp::MatcherOptions options{.allow_overlap = overlap};

    nlp::Workflow pair{.lang = nlp::Language::English,
                       .stages = {nlp::stage::tokenize(),
                                  nlp::stage::matcher({{.label = "AA", .tokens = {"a", "a"}}}, options)}};
    const nlp::Doc repeated = pair.infer("a a a b");

    nlp::Workflow cross{.lang = nlp::Language::English,
                        .stages = {nlp::stage::tokenize(),
                                   nlp::stage::matcher({{.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false},
                                                        {.label = "PAPER", .tokens = {"York", "Times"}, .use_lemmas = false}},
                                                       options)}};
    const nlp::Doc named = cross.infer("New York Times said");

    std::cout << "allow_overlap = " << (overlap ? "true" : "false") << "\n";
    std::cout << "  repeated pattern: " << repeated.matches.size() << " match(es)\n";
    for (const nlp::Span& match : repeated.matches) {
      std::cout << "    [" << match.token_start << ", " << match.token_end << ") \"" << match.text(repeated) << "\"\n";
    }
    std::cout << "  two patterns: " << named.matches.size() << " match(es)\n";
    for (const nlp::Span& match : named.matches) {
      std::cout << "    " << match.label << " \"" << match.text(named) << "\"\n";
    }
  }
  return 0;
}
```

Output

```text
allow_overlap = true
  repeated pattern: 2 match(es)
    [0, 2) "a a"
    [1, 3) "a a"
  two patterns: 2 match(es)
    CITY "New York"
    PAPER "York Times"
allow_overlap = false
  repeated pattern: 1 match(es)
    [0, 2) "a a"
  two patterns: 1 match(es)
    CITY "New York"
```

### Example 3

A lemma-based pattern against inflected text, with the lemmatizer before the
matcher. The pattern `{"ship"}` matches every inflection because each has the
lemma `ship`, and a second pattern with `use_lemmas` off matches only the
exact surface `ship`.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  const char* text = "We ship it; they shipped it, and she ships daily.";
  nlp::MatcherPattern by_lemma{.label = "LEMMA", .tokens = {"ship"}, .use_lemmas = true};
  nlp::MatcherPattern by_surface{.label = "SURFACE", .tokens = {"ship"}, .use_lemmas = false};

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                              nlp::stage::matcher({by_lemma, by_surface})}};
  const nlp::Doc doc = wf.infer(text);

  std::cout << "matches = " << doc.matches.size() << "\n";
  for (const nlp::Span& match : doc.matches) {
    std::cout << match.label << ": " << match.text(doc) << "\n";
  }
  return 0;
}
```

Output

```text
matches = 4
LEMMA: ship
SURFACE: ship
LEMMA: shipped
LEMMA: ships
```

### Example 4

A saved matcher restored into a new stage keeps its patterns and options. The
dump is hex-encoded, so the labels and tokens survive any punctuation or
encoding in the text, and the restored stage finds the same matches.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::MatcherStage original{{.label = "CITY", .tokens = {"New", "York"}, .use_lemmas = false}};
  const std::vector<std::string> saved = original.dump_state_lines();
  std::cout << "saved lines = " << saved.size() << "\n";
  for (const std::string& line : saved) std::cout << "  " << line << "\n";

  const std::vector<std::string_view> views(saved.begin(), saved.end());
  auto restored = std::make_shared<nlp::MatcherStage>();
  restored->load_state_lines(views);

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize(), restored}};
  const nlp::Doc doc = wf.infer("Ada moved to New York.");
  std::cout << "restored matches = " << doc.matches.size() << " label = " << doc.matches[0].label << "\n";
  return 0;
}
```

Output

```text
saved lines = 6
  lowercase=1
  allow_overlap=1
  pattern_count=1
  pattern_label_0=43495459
  pattern_use_lemmas_0=0
  pattern_tokens_0=4e6577,596f726b
restored matches = 1 label = CITY
```

## See Also

- `stage::matcher` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`MatcherPattern`](MatcherPattern.md) -- the rule record this stage applies.
- [`LemmatizerStage`](LemmatizerStage.md) -- supplies the lemmas that `use_lemmas` reads.
- [`NerStage`](NerStage.md) -- entity recognition, with its own pattern rules.
- [`Span`](../core/Span.md) -- the match record this stage writes.

## Changelog

- **0.1** -- initial documented surface.
- **0.1 (fix)** -- `allow_overlap = false` now rejects a match that intersects an earlier match from any pattern, not only from the same pattern.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
