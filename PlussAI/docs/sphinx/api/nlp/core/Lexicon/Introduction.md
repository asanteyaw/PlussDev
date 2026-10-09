# `pluss::ai::nlp::Lexicon` Introduction

## What A Lexicon Is

A `Lexicon` is the vocabulary a document builds as it is tokenized. Every
distinct string gets one stable id from its [`TextPool`](../TextPool.md),
every distinct analysis gets one id from its [`MorphBank`](../MorphBank.md),
and every distinct surface form gets one [`Lexeme`](../Lexeme.md) that
records both. A `Doc` owns one lexicon, so the vocabulary is scoped to the
document it came from.

## Interning Versus Observing

Two operations look similar but mean different things. `intern` records a
bare string in the text pool and returns its id; it does not create a
lexeme. `observe` records a surface form as a lexeme, interning its text,
lowercase, prefix, suffix, and morphology along the way. Tokenization uses
`observe`, so every token's surface form has a lexeme afterward; a caller
interning strings by hand gets ids without lexemes.

That is why `size()` (lexeme count) and `texts().size()` (interned string
count) differ after the same calls. Observing one word interns several
strings, because the lowercase form, affixes, and shape are all stored.

## Ids And Identity

Equal strings always get equal ids, and `observe` returns the same lexeme
object every time it sees the same surface text. Keep the ids and the
references only as long as the lexicon itself lives; a [`clear`](clear.md)
invalidates them all.

## Related Pages

- [`Doc`](../Doc.md) -- the document whose `lexicon` this is.
- [`Lexeme`](../Lexeme.md) -- the record `observe` returns.
- [`Morphology`](../Morphology.md) -- the analysis `intern_morphology`
  stores.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="clear.html">
    <span class="label">Next</span>
    <span class="title">clear</span>
  </a>
</div>
<!-- apiref:nav:end -->
