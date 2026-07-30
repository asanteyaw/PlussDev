<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb52dfd86097 -->

# Catalog::apply_all_

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::apply_all_(const std::function<Stream(const Stream&)>& fn) -> Catalog&
```

## What It Does

The in-place counterpart to [`apply_all()`](apply_all.md) — implemented as
`*this = transform(fn); return *this;`. Mutates `*this` directly and returns a reference to
it, rather than returning a new `Catalog`.

## Parameters

**fn** (`const std::function<Stream(const Stream&)>&`)
    Called once per stream, in turn; must return a defined `Stream` the same length as its
    input, for every stream.

## Returns

`Catalog&`
    A reference to `*this`, after the update.

## Examples

Example 1. Transform every stream in place.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

ct.apply_all_([](const pluss::kindle::Stream& s) {
  if (s.kind() != pluss::kindle::StreamKind::Series) return s;
  return pluss::kindle::Stream(s.as_series() + 1.0);
});
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [2, 3, 4]                                        
metric                   | Series       | Series[float64]  | 3       | [11, 21, 31]                                     
========================================================================================================================
```

## Notes

- Implemented as `*this = transform(fn)` — every throw condition documented on
  [`transform()`](transform.md) applies here unchanged.
- `fn` runs against every stream unconditionally; branch on `Stream::kind()` inside the
  callback to skip kinds you don't want to touch.

## See Also

- [`Catalog::apply_all`](apply_all.md) / [`Catalog::transform`](transform.md): the
  non-mutating forms this method wraps.
- [`Catalog::apply_`](apply_.md): the single-stream, in-place counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_all.html">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
