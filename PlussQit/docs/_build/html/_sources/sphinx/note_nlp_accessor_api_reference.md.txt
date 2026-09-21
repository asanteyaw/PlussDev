<!-- pluss-hand-authored -->
# Note NLP Accessor API Reference

[Back to Note API Reference](note_api_reference)

`Note::nlp()` is the linguistic operation boundary for Note. It keeps generic
text mechanics on `Note` while collecting operations whose meaning depends on
tokens, sentences, language, document markup, corpus cleanup, phonetics, or
linguistic set comparison.

The accessor is zero-overhead plumbing: obtaining it performs no transform and
does not enlarge `Note`. Every terminal method is still batched over all cells
and returns either a shape-preserving Note or a closed `TokenBatch` containing
Text tokens and Int64 offsets.

[`TokenBatch`](tokenbatch_api_reference.md) has its own method reference. Use
it to learn the flattened token Note, cumulative offsets, defined/empty state,
and borrowed-reference lifetime shared by every ragged NLP result.

## Choosing A Family

| Need | Accessor family | Result |
|---|---|---|
| Normalize prose for later analysis | `clean`, `normalize_*`, `expand_contractions` | Shape-preserving Text Note |
| Delete or mask recognized spans | `remove_*`, `mask_*` | Shape-preserving Text Note |
| Detect language or score prose | `language`, `script`, sentiment/readability methods | Shape-preserving Text or Float64 Note |
| Extract variable-length units | token, sentence, entity, keyword, and citation methods | `TokenBatch` |
| Transform token sequences | filtering, ordering, deduplication, and n-grams | `TokenBatch` |
| Compare linguistic sets or align sequences | `intersect_*`, `jaccard_*`, `overlap_*`, `diff_words`, `align_*` | `TokenBatch` or Float64 Note |
| Produce language-oriented representations | transliteration and phonetic methods | Shape-preserving Text Note |

## Canonical Surface

The accessor deliberately omits duplicate spellings. `tokens()` is the word
tokenizer; there is no parallel `words()` alias. `sentences()` is the sentence
tokenizer. `char_ngrams(n)` and `word_ngrams(n)` cover every width, so fixed
`bigrams()` and `trigrams()` aliases are not part of the public surface.

Every public accessor method has one canonical handwritten page below. The list is ordered A-Z, and each page participates in the same family-aware previous/next chain.

```{toctree}
:hidden:
:maxdepth: 1

api/note-nlp/acronyms
api/note-nlp/align_sentences
api/note-nlp/align_tokens
api/note-nlp/alnum_tokens
api/note-nlp/alpha_tokens
api/note-nlp/char_ngrams
api/note-nlp/citations
api/note-nlp/clean
api/note-nlp/definitions
api/note-nlp/diff_words
api/note-nlp/emails
api/note-nlp/expand_contractions
api/note-nlp/formality
api/note-nlp/hashtags
api/note-nlp/intersect_acronyms
api/note-nlp/intersect_citations
api/note-nlp/intersect_symbols
api/note-nlp/intersect_terms
api/note-nlp/intersect_tokens
api/note-nlp/jaccard_acronyms
api/note-nlp/jaccard_citations
api/note-nlp/jaccard_symbols
api/note-nlp/jaccard_terms
api/note-nlp/jaccard_tokens
api/note-nlp/keyphrases
api/note-nlp/keywords
api/note-nlp/language
api/note-nlp/lemmas
api/note-nlp/lower_tokens
api/note-nlp/mask_currency
api/note-nlp/mask_dates
api/note-nlp/mask_emails
api/note-nlp/mask_hashtags
api/note-nlp/mask_mentions
api/note-nlp/mask_numbers
api/note-nlp/mask_pii
api/note-nlp/mask_times
api/note-nlp/mask_urls
api/note-nlp/match_tokens
api/note-nlp/math_symbols
api/note-nlp/max_length
api/note-nlp/mentions
api/note-nlp/metaphone
api/note-nlp/min_length
api/note-nlp/named_entities
api/note-nlp/normalize_numbers
api/note-nlp/normalize_punct
api/note-nlp/numbers
api/note-nlp/overlap_acronyms
api/note-nlp/overlap_citations
api/note-nlp/overlap_symbols
api/note-nlp/overlap_terms
api/note-nlp/overlap_tokens
api/note-nlp/pos_tags
api/note-nlp/punct_tokens
api/note-nlp/readability_grade
api/note-nlp/readability_level
api/note-nlp/remove_boilerplate
api/note-nlp/remove_brackets
api/note-nlp/remove_bullets
api/note-nlp/remove_code
api/note-nlp/remove_emails
api/note-nlp/remove_footers
api/note-nlp/remove_hashtags
api/note-nlp/remove_headers
api/note-nlp/remove_html
api/note-nlp/remove_latex
api/note-nlp/remove_markdown
api/note-nlp/remove_math
api/note-nlp/remove_mentions
api/note-nlp/remove_parentheticals
api/note-nlp/remove_quotes
api/note-nlp/remove_stopwords
api/note-nlp/remove_tables
api/note-nlp/remove_urls
api/note-nlp/script
api/note-nlp/sentences
api/note-nlp/sentiment
api/note-nlp/shuffle_tokens
api/note-nlp/sort_tokens
api/note-nlp/soundex
api/note-nlp/spam_score
api/note-nlp/stems
api/note-nlp/subjectivity
api/note-nlp/terms
api/note-nlp/to_ascii
api/note-nlp/to_cyrillic
api/note-nlp/to_greek
api/note-nlp/tokens
api/note-nlp/toxicity
api/note-nlp/unique_tokens
api/note-nlp/urls
api/note-nlp/without_numbers
api/note-nlp/without_stopwords
api/note-nlp/word_ngrams
```

## Result Types And Row Boundaries

Shape-preserving analyses and cleanup methods return `Note`. Variable-length
extractors and token transforms return `TokenBatch`; use `batch.tokens()` for
the flat Text payload and `batch.offsets()` for cumulative cell boundaries.
Neither accessor belongs to root `Note`, which is why `offsets` appears only in
the standalone TokenBatch sidebar.
