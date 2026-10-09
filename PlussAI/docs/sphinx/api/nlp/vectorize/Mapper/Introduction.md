# The Learned Mapper

A `Mapper` is the classical bag-of-words vectorizer for this library. Its life has two phases.
In the first, [`learn`](learn.md) reads training documents, extracts their terms under the
[`DocMapOptions`](../DocMapOptions.md) rule, counts how many documents each term appears in, and
keeps the most useful terms as the vocabulary. In the second, the counting and weighting methods
turn documents into rows over that vocabulary, and a term the vocabulary does not hold contributes
nothing.

Every mapping method takes the same documents-in form, so a pipeline that learns on one set and maps
another reads the same way. The choices that matter most are the vocabulary's size and
selectivity, which [`LexiconMapOptions`](../../../classic/text/LexiconMapOptions.md) controls through
`max_features` and `min_document_frequency`, and the weighting, which the `tfidf_*` methods control
through [`TfIdfMapOptions`](../../../classic/text/TfIdfMapOptions.md).

The mapper is the right choice when the columns must be named, when the vocabulary can be stored, and
when TF-IDF weighting is wanted. Its state persists through `dump_state` and `load_state`, so a
trained mapper can be restored in another process without re-reading the training corpus.

## See Also

- [`learn`](learn.md) -- builds the vocabulary.
- [`count_dense`](count_dense.md) and [`tfidf_dense`](tfidf_dense.md) -- the dense mapping methods.
- [`Hasher`](../Hasher.md) -- the alternative when a fixed width matters more than named columns.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="count_dense.html">
    <span class="label">Next</span>
    <span class="title">count_dense</span>
  </a>
</div>
<!-- apiref:nav:end -->
