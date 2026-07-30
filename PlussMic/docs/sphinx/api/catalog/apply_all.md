<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 684eed1937c2 -->

# Catalog::apply_all

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::apply_all(const std::function<Stream(const Stream&)>& fn) const -> Catalog
```

## What It Does

Applies the same function to **every** stream, non-mutating — a pure alias for
[`transform(fn)`](transform.md) (the one-argument overload). See
[`apply()`](apply.md)'s page for the full one-stream-vs-every-stream /
non-mutating-vs-in-place matrix this method fits into.

```{important} A pure alias, not a separate implementation
`apply_all(fn)` and `transform(fn)` (one argument) call the exact same code — there is no
behavioral difference whatsoever between them. Pick whichever name reads better; neither
is more "correct" or more efficient than the other.
```

Because it runs `fn` against every stream regardless of payload kind, `fn` typically needs
to branch on `Stream::kind()` if it should only affect some streams — see Example 1.

## Parameters

**fn** (`const std::function<Stream(const Stream&)>&`)
    Called once per stream, in turn; must return a defined `Stream` the same length as its
    input, for every stream.

## Returns

`Catalog`
    A new catalog with every stream replaced by `fn`'s result for that stream.

## Examples

Example 1. Add `1` to every numeric stream, leaving text streams untouched by checking
`kind()` inside the callback.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.apply_all([](const pluss::kindle::Stream& s) {
  if (s.kind() != pluss::kindle::StreamKind::Series) return s;
  return pluss::kindle::Stream(s.as_series() + 1.0);
});
std::cout << out;
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

- A pure alias for [`transform(fn)`](transform.md) — see the important callout above.
- `fn` runs against every stream unconditionally; branch on `Stream::kind()` inside the
  callback to skip kinds you don't want to touch.
- Every returned stream must be defined and the same length as its input, or the call
  throws.

## See Also

- [`Catalog::transform`](transform.md): the method this is a pure alias for.
- [`Catalog::apply`](apply.md): the single-stream counterpart.
- [`Catalog::apply_all_`](apply_all_.md): the in-place counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_.html">
    <span class="label">Previous</span>
    <span class="title">apply_</span>
  </a>
  <a class="next-link" href="apply_all_.html">
    <span class="label">Next</span>
    <span class="title">apply_all_</span>
  </a>
</div>
