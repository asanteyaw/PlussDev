<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 73b0cadcf715 -->

# Catalog::transform

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::transform(const std::function<Stream(const Stream&)>& fn) const -> Catalog
Catalog::transform(std::string_view field, const std::function<Stream(const Stream&)>& fn) const -> Catalog
```

## What It Does

Two overloads sharing one name, with different scope:

- **`transform(fn)`** (one argument) transforms **every** stream. It's the primary
  implementation that [`apply_all()`](apply_all.md) is a pure alias for.
- **`transform(field, fn)`** (two arguments) transforms **one** named stream. It's a pure
  alias that forwards straight to [`apply(field, fn)`](apply.md).

```{important} This is the actual implementation apply_all() aliases — the reverse of what the name order suggests
Reading the source, `apply_all(fn)` is literally `return transform(fn);` — `transform()`
is the real implementation, `apply_all()` is the alias. The two-argument overload runs the
other direction: `transform(field, fn)` is literally `return apply(field, fn);` — there,
`apply()` is the real implementation and `transform()` is the alias. Which name is "the
real one" flips depending on which overload you're looking at; functionally it makes no
difference either way.
```

## Parameters

**fn** (`const std::function<Stream(const Stream&)>&`)
    Called once per stream (one-argument overload) or once for the named stream
    (two-argument overload); must return a defined `Stream` of the same length as its
    input.

**field** (`std::string_view`, two-argument overload only)
    The single stream name to transform. Must already exist.

## Returns

`Catalog`
    A new catalog with the affected stream(s) replaced by `fn`'s result(s); every other
    stream unchanged.

## Examples

Example 1. The one-argument overload transforms every stream — same result as
[`apply_all()`](apply_all.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.transform([](const pluss::kindle::Stream& s) {
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

Example 2. The two-argument overload transforms only the named stream — same result as
[`apply()`](apply.md).

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.transform("id", [](const pluss::kindle::Stream& s) {
  return pluss::kindle::Stream(s.as_series() * 2.0);
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
id                       | Series       | Series[float64]  | 3       | [2, 4, 6]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

## Notes

- Overload count (one vs. two arguments), not a runtime flag, decides scope — see the
  important callout above for exactly which method aliases which.
- The two-argument overload requires `field` to already exist, same as
  [`apply()`](apply.md).
- Every returned stream must be defined and the same length as its input.

## See Also

- [`Catalog::apply_all`](apply_all.md): a pure alias for the one-argument overload.
- [`Catalog::apply`](apply.md) / [`Catalog::apply_`](apply_.md): the single-stream forms —
  `apply()` is a pure alias for this method's two-argument overload.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_table.html">
    <span class="label">Previous</span>
    <span class="title">to_table</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
