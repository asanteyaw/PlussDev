# Language Data In The Pipeline

Each language's data is a small set of lookups. Abbreviations tell the sentencizer that a period belongs to a word,
so `Dr. Ada` does not end a sentence. Titles tell the entity recognizer that a capitalized name follows. Lemma rules
supply base forms the rule-based lemmatizer cannot guess. Token merges rejoin pieces that the splitter separated, such
as a spaced honorific.

Lookups are deliberately narrow. They are exact lists, matched case-insensitively for abbreviations and titles and
case-sensitively for merges, so the data is easy to read and to audit. A form that is not listed is simply not
treated specially, which is the safe default.

## See Also

- [`pack`](pack.md) -- returns a language's data.
- [`lookup_lemma`](lookup_lemma.md) -- the lemma lookup the lemmatizer uses.

## Changelog

- **0.1** -- initial documented surface.
