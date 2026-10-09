# `pluss::ai::nlp::Span`

A labeled token range inside a [`Doc`](Doc.md) -- the one shape behind
sentences, phrases, matches, entities, and dependency branches. A `Span`
stores only token indices and a label; its surface text is computed on
demand from the owning `Doc`, so it never duplicates the document's
text.

## Signature

```cpp
pluss::ai::nlp::Span::Span() = default;
```

Spans are plain aggregates: callers name the three fields directly
(`Span{.token_start = 0, .token_end = 2, .label = "x"}`), and every
annotation stage builds its own spans this way.

## Construction

Aggregate initialization from `token_start` (inclusive), `token_end`
(exclusive), and `label`. A default-constructed `Span` covers no tokens
and is `empty()`. In practice a caller reads spans out of a `Doc`
(`doc.sentences`, `doc.matches`, `doc.entities`, or the return value of
[`branch_of`](Doc/branch_of.md)) rather than building them.

## State

`token_start`
: Index of the first token in the span. Inclusive.

`token_end`
: One past the last token in the span. Exclusive, so a span over tokens
  `2` and `3` has `token_start == 2` and `token_end == 4`.

`label`
: A free-form string naming what the span is -- `"ORG"` for an entity
  from [`NerStage`](../tasks/NerStage.md), `"action"` for a
  [`MatcherStage`](../tasks/MatcherStage.md) match, `"branch"` for a span
  [`Doc::branch_of`](Doc/branch_of.md) computed. Stages set it; nothing
  in `core` interprets it.

## Methods

### `empty() const`

Returns `true` when `token_end <= token_start`, meaning the span covers
no tokens. Use it to skip an out-of-range result from
[`branch_of`](Doc/branch_of.md) without inspecting the two indices
separately.

Returns `bool`. Never throws.

### `text(const Doc&) const`

Returns the substring of `doc.text` that the span covers, from the
first token's `char_start` to the last token's `char_end`. Returns an
empty string when the span is empty, when either index falls outside
`doc.tokens`, or when the character offsets do not describe a valid
range -- a defensive fallback for a span paired with the wrong `Doc`.

Returns `std::string`. Never throws.

## Practical Examples

These examples build spans by hand over one tokenized sentence, so the
index arithmetic is visible.

### Example 1

Reading a two-token span's text, then checking the empty test on a
zero-width span.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  const nlp::Span first_two{.token_start = 0, .token_end = 2, .label = "pair"};
  const nlp::Span nothing{.token_start = 3, .token_end = 3, .label = "none"};

  std::cout << "text=" << first_two.text(doc) << "\n";
  std::cout << "empty=" << first_two.empty() << "\n";
  std::cout << "nothing_empty=" << nothing.empty() << "\n";
  return 0;
}
```

Output

```text
text=Laurel writes
empty=0
nothing_empty=1
```

### Example 2

A span covering the whole document, and the label carrying through
unchanged.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  const nlp::Span everything{
      .token_start = 0,
      .token_end = static_cast<std::int64_t>(doc.tokens.size()),
      .label = "all",
  };
  std::cout << "label=" << everything.label << " text=" << everything.text(doc) << "\n";
  return 0;
}
```

Output

```text
label=all text=Laurel writes APIs.
```

### Example 3

An out-of-range span returns an empty string instead of reading past
the token list.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English, .stages = {nlp::stage::tokenize()}};
  const nlp::Doc doc = wf.infer("Laurel writes APIs.");

  const nlp::Span beyond{.token_start = 2, .token_end = 99, .label = "bad"};
  std::cout << "text=[" << beyond.text(doc) << "]\n";
  return 0;
}
```

Output

```text
text=[]
```

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
