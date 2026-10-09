# `pluss::ai::nlp` Resources

The resources are the language data the text stages consult: which tokens are abbreviations, which are
titles, which inflected forms have a known base form, and which adjacent token pairs form a single unit. They
are fixed per language and ship with the library. The sentencizer, lemmatizer, tokenizer, and entity
recognizer all read them, so the same language behaves the same way across stages.

The language data covers English, French, Spanish, and German. A document whose language is unknown uses an empty
pack, so the stages fall back to their rule-only behavior rather than applying another language's data.

## See Also

- [`Introduction`](resources/Introduction.md) -- how the language data is used.
- [`LanguagePack`](resources/LanguagePack.md) -- the record that holds one language's data.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

resources/Introduction
resources/LemmaRule
resources/TokenMergeRule
resources/LanguagePack
resources/pack
resources/available_languages
resources/parse_language_code
resources/language_code
resources/language_name
resources/is_abbreviation
resources/is_title
resources/lookup_lemma
resources/try_merge_token_exception
```
<!-- apiref:toctree:end -->
