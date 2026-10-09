# `pluss::ai::nlp::Lexicon`

The type-level lexical state of a document: one [`TextPool`](TextPool.md)
that interns every string, one [`MorphBank`](MorphBank.md) that
deduplicates every analysis, and one [`Lexeme`](Lexeme.md) per distinct
surface form observed so far. A [`Doc`](Doc.md) owns its lexicon through
`Doc::lexicon`; tokenization fills it in.

## Signature

```cpp
pluss::ai::nlp::Lexicon::Lexicon() = default;
```

## Construction

A default-constructed lexicon is empty: no strings, no morphology, no
lexemes. Its internal morphology bank is attached to its own text pool at
construction, so the two always agree on ids. In normal use a caller
reaches the lexicon through `doc.lexicon`; a standalone lexicon is useful
for building vocabularies outside a document.

## State

A lexicon holds three things, each reachable through its own accessor:

- the text pool (`texts()`), which assigns a `StringId` to every
  string it stores;
- the morphology bank (`morphs()`), which stores one
  [`Morphology`](Morphology.md) per feature class;
- the lexemes, one per distinct surface text, returned by
  [`observe`](Lexicon/observe.md) and [`find`](Lexicon/find.md).

`size()` counts lexemes, not interned strings. Interning a string on its
own adds to `texts().size()` but leaves `size()` unchanged.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `intern(std::string_view)` | `StringId` | [`intern`](Lexicon/intern.md) |
| `lookup(std::string_view) const` | `StringId` | [`lookup`](Lexicon/lookup.md) |
| `text(StringId) const` | `const std::string&` | [`text`](Lexicon/text.md) |
| `observe(std::string_view)` | `const Lexeme&` | [`observe`](Lexicon/observe.md) |
| `find(...) const` | `const Lexeme*` | [`find`](Lexicon/find.md) |
| `intern_morphology(const Morphology&)` | `StringId` | [`intern_morphology`](Lexicon/intern_morphology.md) |
| `find_morphology(StringId) const` | `const Morphology*` | [`find_morphology`](Lexicon/find_morphology.md) |
| `size() const` | `std::size_t` | [`size`](Lexicon/size.md) |
| `texts() const` | `const TextPool&` | [`texts`](Lexicon/texts.md) |
| `morphs() const` | `const MorphBank&` | [`morphs`](Lexicon/morphs.md) |
| `clear()` | `void` | [`clear`](Lexicon/clear.md) |

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Lexicon/Introduction
Lexicon/clear
Lexicon/find
Lexicon/find_morphology
Lexicon/intern
Lexicon/intern_morphology
Lexicon/lookup
Lexicon/morphs
Lexicon/observe
Lexicon/size
Lexicon/text
Lexicon/texts
```
<!-- apiref:toctree:end -->
