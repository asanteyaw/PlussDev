# Vectorization Pipeline

Every vectorizer in this family starts from the same step: each document is reduced
to a sequence of feature strings, and the vectorizer counts those strings. Choosing
what goes into that sequence is the most consequential decision in the pipeline, so
this page describes it before the vectorizers themselves.

## The Feature Text

For each document, the vectorizer walks `Doc::tokens` in order. A punctuation token is
dropped when [`skip_punct`](DocMapOptions.md) is on. Each remaining token contributes its
lemma when [`use_lemmas`](DocMapOptions.md) is on and the lemma is non-empty, and its
surface form otherwise. The pieces are joined with single spaces. A document with no
tokens at all, such as one that was never tokenized, contributes its raw `Doc::text`
instead, so its words still count.

The joined text is then split again on whitespace by the vectorizer, with lowercasing
and n-gram expansion governed by the vectorizer's own options. An n-gram is built from
adjacent pieces, so a bigram such as `cat sat` is a column only after that expansion.

## Learning Versus Mapping

A `Mapper` separates two phases. `learn()` reads a corpus and fixes the vocabulary and
the document frequency of every term. The mapping methods then turn any document into a
row over that fixed vocabulary, so a document with an unseen term loses that term. A
`Hasher` skips the first phase: every term maps to a bucket by hashing, so no
vocabulary exists and no term is ever dropped.

Because learning fixes the columns, the same vectorizer must produce the training and the
inference matrices. Persist the vocabulary with `dump_state()` and restore it with
`load_state()` when the two happen in different processes.

## Corpus Statistics

[`describe`](describe.md) runs the same feature-text rule and reports what it finds
without producing a matrix. Use it before learning to see how many distinct terms the
corpus has, and how many of them appear in only one or two documents, which is what
`min_document_frequency` removes.

## See Also

- [`Mapper`](Mapper.md) -- the learned vectorizer.
- [`Hasher`](Hasher.md) -- the fixed-width vectorizer.
- [`DocMapOptions`](DocMapOptions.md) -- the feature-text choices.
- [`describe`](describe.md) -- corpus statistics without a matrix.

## Changelog

- **0.1** -- initial documented surface.
