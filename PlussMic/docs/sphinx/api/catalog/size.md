<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Catalog::size

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::size() const -> std::int64_t
```

## What It Does

A pure alias for [`nkeys()`](nkeys.md) — returns the number of named streams in the
catalog. `size()` exists for readability at call sites that expect a generic
"how big is this container" query; `nkeys()` exists for call sites where "how many streams"
is the more precise framing. There is no behavioral difference.

```{important} Counts streams, not rows
Like `nkeys()`, `size()` reports the catalog's *width* (how many named streams it holds),
not the length of any individual stream. For row counts, see
[`nrows()`](nrows.md)/[`minrows()`](minrows.md)/[`maxrows()`](maxrows.md).
```

## Parameters

`None`

## Returns

`std::int64_t`
    The number of named streams in the catalog — identical to `nkeys()`.

## Examples

Example 1. `size()` and `nkeys()` report the same value.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << ct.size() << " == " << ct.nkeys();
```

### Output

```text
3 == 3
```

Example 2. An explicitly empty catalog reports `size() == 0`.

```cpp
pluss::kindle::Catalog ct(std::vector<std::pair<std::string, pluss::kindle::Stream>>{});
std::cout << ct.size();
```

### Output

```text
0
```

## Notes

- A pure alias for [`nkeys()`](nkeys.md) — no behavioral difference, pick whichever reads
  better.
- Counts streams, not rows — see the important callout above.

## See Also

- [`Catalog::nkeys`](nkeys.md): the method this is an alias for.
- [`Catalog::shape`](shape.md): both dimensions (rows, streams) in one call.
- [`Catalog::empty`](empty.md): shorthand for `size() == 0`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
