<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 693bbbbd387b -->

# Catalog::query

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::query(const QueryFn& fn) const -> Catalog
```

```cpp
using QueryFn = std::function<pluss::Series(const Catalog&)>;
```

## What It Does

Builds the filter mask *from the catalog itself*, instead of requiring you to construct it
beforehand — `query(fn)` calls `fn(*this)` to get a boolean `Series`, then applies it
exactly the way [`filter()`](filter.md) would. This is the natural way to express a
row-selection condition that references one or more of the catalog's own streams without
first pulling them out into local variables.

## Parameters

**fn** (`const QueryFn&`, i.e. `std::function<pluss::Series(const Catalog&)>`)
    A callback that receives the catalog and returns a `Bool`-dtype mask `Series`. Must not
    be empty.

## Returns

`Catalog`
    A new catalog with every stream restricted to the rows where `fn(*this)` returned
    `true`.

## Examples

Example 1. Select rows where one of the catalog's own streams exceeds a threshold, without
naming an intermediate mask variable.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto out = ct.query([](const pluss::kindle::Catalog& c) {
  return c.at("id").as_series() > 2.0;
});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [3, 4, 5]                                        
city                     | Note         | Note             | 3       | [Kumasi, Ho, Cape Coast]                         
========================================================================================================================
```

Example 2. An empty (default-constructed) `std::function` throws immediately, before
attempting to call it.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  Catalog::QueryFn empty_fn;
  auto out = ct.query(empty_fn);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::query: query function is empty
```

## Notes

- `fn` receives `*this` and must return a `Bool`-dtype `Series` — everything downstream is
  identical to calling [`filter()`](filter.md) with that returned mask directly, including
  the per-stream length check.
- `Catalog::QueryFn` is a plain `std::function<pluss::Series(const Catalog&)>` — any
  callable with that signature works, not just lambdas.

## See Also

- [`Catalog::filter`](filter.md) / [`Catalog::where`](where.md) /
  [`Catalog::where`](where.md): the underlying row-selection mechanism this
  method builds its mask for.
- [`Catalog::at`](at.md): the usual way to reach a stream inside the callback.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrows.html">
    <span class="label">Previous</span>
    <span class="title">nrows</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
