# `nlp::core` Introduction

## What This Category Owns

`core` owns every type the rest of `nlp` is built from: [`Doc`](Doc.md)
(one processed document), [`Token`](Token.md)/[`Span`](Span.md) (its
annotation units), [`Lexicon`](Lexicon.md)/[`Lexeme`](Lexeme.md)/
[`TextPool`](TextPool.md)/[`MorphBank`](MorphBank.md)/[`Morphology`](Morphology.md)
(the type-level lexical state a `Doc` accumulates as it is processed),
and [`IStage`](IStage.md) (the contract every pipeline stage implements).
No other `nlp` category introduces a new core type -- [`workflow`](../workflow.md)
orchestrates `Doc`s through `IStage`s, [`tasks`](../tasks.md) supplies
concrete `IStage` implementations, and every other category either
produces or consumes a `Doc`.

## Occurrence Vs. Type: Token Vs. Lexeme

A [`Token`](Token.md) is one occurrence in one `Doc` -- it has a
position (`char_start`/`char_end`, `index`), and its own
per-occurrence annotations (`pos`, `role`, `entity_label`,
`sentence_start`/`sentence_end`). A [`Lexeme`](Lexeme.md) is the
type-level record `Lexicon` keeps once per distinct surface form,
regardless of how many times it occurs. Both carry a
[`Morphology`](Morphology.md), but a token's own morphology is cached
once at tokenization time and then shared by string id with every other
occurrence of the same surface form -- `Lexicon::intern_morphology`
deduplicates it, so `Token::morphology_id` repeats the exact same value
across every repeated occurrence of one surface form in the same `Doc`
(see [`Lexicon`](Lexicon.md)'s own worked example).

## Interning: Why `StringId`, Not `std::string`, On Hot Paths

[`TextPool`](TextPool.md) interns every distinct string a `Lexicon` sees
exactly once and hands back a stable `StringId` (a plain `std::uint64_t`)
for it. `Token`/`Lexeme` carry both the interned id (`orth_id`,
`lower_id`, ...) and, for convenience, the actual string (`orth`,
`lower`, ...) -- the id is what a performance-sensitive consumer (a
[`tasks`](../tasks.md) stage comparing many tokens, or
[`vectorize`](../vectorize.md) building a vocabulary) should compare,
since two occurrences of the same surface form always share one id by
construction, while comparing the strings themselves re-does the
character comparison every time.

## Reading Order

1. [`Doc`](Doc.md) -- the one type every other page in this reference
   ultimately produces or consumes.
2. [`Token`](Token.md) / [`Span`](Span.md) -- `Doc`'s own annotation
   units.
3. [`Morphology`](Morphology.md) -- the shape/casing analysis both
   `Token` and `Lexeme` carry.
4. [`Lexeme`](Lexeme.md) / [`TextPool`](TextPool.md) /
   [`MorphBank`](MorphBank.md) / [`Lexicon`](Lexicon.md) -- the
   type-level lexical state `Doc::lexicon` accumulates.
5. [`IStage`](IStage.md) -- the contract behind every
   [`tasks`](../tasks.md) stage, for a reader who wants to write a
   custom one.

## Related Pages

- [`workflow`](../workflow.md) -- `Workflow`, what actually runs a
  pipeline of `IStage`s over `Doc`s.
- [`tasks`](../tasks.md) -- the built-in stage catalog.
