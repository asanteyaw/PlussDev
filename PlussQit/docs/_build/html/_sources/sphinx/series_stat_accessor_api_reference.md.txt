<!-- pluss-hand-authored -->

# SeriesStatAccessor API Reference

[Back to Series API Reference](series_api_reference.md)

This page documents the handwritten `SeriesStatAccessor` subset reached through
`Series::stat`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, failure
  cases, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one
  giant scroll of every statistical method at once.

## Entry Surface

The accessor is available in two ordinary spellings:

- dot-style convenience: `x.stat.var(...)`, `x.stat.center(...)`
- explicit proxy style: `x.stat().var(...)`, `x.stat().center(...)`

For the current handwritten batch, the dot-style forms are the main examples
because they are the most concise everyday spelling on the public surface.

## Current Coverage

The current handwritten `SeriesStatAccessor` subset focuses on descriptive and
normalization-heavy methods users typically reach first:

- dispersion: `var`, `std`, `nanvar`, `nanstd`
- transforms: `center`, `standardize`
- higher moments: `skewness`, `kurtosis`

The accessor surface is wider than this. Pair-bound distance methods, density
helpers, and moving-window methods should be documented in later batches rather
than stubbed here without the same example rigor.

```{toctree}
:hidden:
:maxdepth: 1

api/series/var
api/series/std
api/series/nanvar
api/series/nanstd
api/series/center
api/series/standardize
api/series/skewness
api/series/kurtosis
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="api/series/stat.html">
    <span class="label">Previous</span>
    <span class="title">stat</span>
  </a>
</div>
