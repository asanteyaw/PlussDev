# The Hashed Vectorizer

A `Hasher` replaces the vocabulary with arithmetic. Each term is hashed to a number, and the number,
reduced modulo `n_features`, names the column the term adds to. Because the mapping is a fixed function
of the term, the hasher never needs to see a corpus before it maps one, and a term it has never met
still lands in some column.

## What Collisions Cost

Two terms that hash to the same column add into one feature, and a model cannot tell them apart. The
probability of a collision grows with the number of distinct terms relative to `n_features`. A wide
hasher keeps collisions rare; a narrow one trades memory for accuracy. The
`alternate_sign` option reduces the bias collisions introduce: each term adds with a sign taken from
its hash, so colliding terms partly cancel rather than always adding up.

## When To Use It

Use a hasher when the vocabulary is open-ended, when the corpus is too large to hold a vocabulary, or
when inputs arrive as a stream and must be vectorized without a fitting pass. Use a
[`Mapper`](../Mapper.md) when the columns must be named, when TF-IDF weighting is needed, or when the
vocabulary is small enough that exact columns cost nothing.

## See Also

- [`dense`](dense.md) and [`sparse`](sparse.md) -- the mapping methods.
- [`Mapper`](../Mapper.md) -- the learned, named alternative.
- [`HashMapOptions`](../../../classic/text/HashMapOptions.md) -- the hashing configuration.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="dense.html">
    <span class="label">Next</span>
    <span class="title">dense</span>
  </a>
</div>
<!-- apiref:nav:end -->
