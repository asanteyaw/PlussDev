# `pluss::ai::nlp::Doc`

One processed document: raw text plus every annotation layer a
[`Workflow`](../workflow/Workflow.md)'s stages add -- tokens, sentences,
phrases, matches, entities, and (for a textcat-bearing pipeline) a
predicted label or label set.

## Signature

```cpp
pluss::ai::nlp::Doc::Doc() = default;
```

## Construction

A `Doc` is almost always produced by [`Workflow::infer`](../workflow/Workflow/infer.md)
or [`Workflow::learn`](../workflow/Workflow/learn.md) -- a caller rarely
constructs one directly. `Doc{}` (default construction) gives an empty
document with its own fresh [`Lexicon`](Lexicon.md); `text`/`lang` can
then be set by hand before passing it through a `Workflow` overload
that accepts pre-built `Doc`s directly (the `learn(const
std::vector<Doc>&, ...)` shape -- see
[`Workflow::learn`](../workflow/Workflow/learn.md)).

## State

`text`
: The document's raw, unmodified input string -- every `Span`'s
  [`text`](Span.md) call slices directly out of this.

`lang`
: The `Language` the pipeline ran with, default `English`. Every
  language-specific stage ([`SentencizerStage`](../tasks/SentencizerStage.md)'s
  abbreviation list, [`LemmatizerStage`](../tasks/LemmatizerStage.md)'s
  lookup table) reads this to pick the right
  [`resources::LanguagePack`](../resources/LanguagePack.md).

`lexicon`
: A `std::shared_ptr<Lexicon>`, already pointing at a fresh, empty
  `Lexicon` by default -- a `Doc` never starts with a null `lexicon`
  unless a caller explicitly clears the field by hand. Tokenization
  fills it in as a side effect; see
  [`ensure_lexicon`](Doc/ensure_lexicon.md)/[`maybe_lexicon`](Doc/maybe_lexicon.md).

`tokens`
: The per-occurrence [`Token`](Token.md) list every stage after
  tokenization reads and extends in place -- filling in `pos`, `role`,
  `entity_label`, and the sentence-boundary flags as the relevant
  stages run.

`sentences` / `phrases` / `matches` / `entities`
: Four parallel `std::vector<Span>` annotation layers, each populated by
  exactly one stage and empty until that stage has run:
  [`SentencizerStage`](../tasks/SentencizerStage.md) fills `sentences`,
  [`SyntaxStage`](../tasks/SyntaxStage.md) fills `phrases` (noun-chunk-style
  branches), [`MatcherStage`](../tasks/MatcherStage.md) fills `matches`,
  and [`NerStage`](../tasks/NerStage.md) fills `entities`.

`doc_label` / `doc_score` / `doc_labels`
: Populated only by a [`TextCatStage`](../tasks/TextCatStage.md).
  `doc_label`/`doc_score` fill in for single-label classification;
  `doc_labels` (a list of label/probability pairs, most confident first)
  fills in instead when the stage's own
  [`TextCatOptions::multi_label`](../tasks/TextCatStage.md) is set --
  see that stage's own page for exactly which field to read for a given
  configuration.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `ensure_lexicon()` | `Lexicon&` | [`ensure_lexicon`](Doc/ensure_lexicon.md) |
| `maybe_lexicon() const` | `const Lexicon*` | [`maybe_lexicon`](Doc/maybe_lexicon.md) |
| `token_texts() const` | `std::vector<std::string>` | [`token_texts`](Doc/token_texts.md) |
| `lemma_texts() const` | `std::vector<std::string>` | [`lemma_texts`](Doc/lemma_texts.md) |
| `sentence_texts() const` | `std::vector<std::string>` | [`sentence_texts`](Doc/sentence_texts.md) |
| `phrase_texts() const` | `std::vector<std::string>` | [`phrase_texts`](Doc/phrase_texts.md) |
| `match_texts() const` | `std::vector<std::string>` | [`match_texts`](Doc/match_texts.md) |
| `entity_texts() const` | `std::vector<std::string>` | [`entity_texts`](Doc/entity_texts.md) |
| `children_of(index) const` | `std::vector<std::int64_t>` | [`children_of`](Doc/children_of.md) |
| `lineage_of(index) const` | `std::vector<std::int64_t>` | [`lineage_of`](Doc/lineage_of.md) |
| `branch_of(index) const` | `Span` | [`branch_of`](Doc/branch_of.md) |
| `left_edge_of(index) const` | `std::int64_t` | [`left_edge_of`](Doc/left_edge_of.md) |
| `right_edge_of(index) const` | `std::int64_t` | [`right_edge_of`](Doc/right_edge_of.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Doc/Introduction
Doc/branch_of
Doc/children_of
Doc/ensure_lexicon
Doc/entity_texts
Doc/left_edge_of
Doc/lemma_texts
Doc/lineage_of
Doc/match_texts
Doc/maybe_lexicon
Doc/phrase_texts
Doc/right_edge_of
Doc/sentence_texts
Doc/token_texts
```
<!-- apiref:toctree:end -->
