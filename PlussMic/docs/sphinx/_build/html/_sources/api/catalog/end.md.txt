<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60f7519b3236 -->

# Catalog::end

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::end() const -> const_iterator
```

## What It Does

Returns the past-the-end iterator over the catalog's **key names**, pairing with
[`begin()`](begin.md) to make `Catalog` usable in range-`for` loops and with standard
algorithms. See [`begin()`](begin.md) for the full explanation of what iteration yields
(key names, not streams or row values) and why this makes `Catalog` interoperate with
`<algorithm>` functions directly.

## Parameters

`None`

## Returns

`const_iterator`
    An iterator one past the last key name. Equal to `begin()` if the catalog has no
    streams.

## Examples

Example 1. Manual iteration with `begin()`/`end()`, instead of range-`for`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

for (auto it = ct.begin(); it != ct.end(); ++it) {
  std::cout << *it << " ";
}
```

### Output

```text
id metric 
```

Example 2. `std::distance(begin(), end())` gives the key count — the same value
[`nkeys()`](nkeys.md) reports directly, without iterators.

```cpp
#include <iterator>

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << std::distance(ct.begin(), ct.end());
```

### Output

```text
3
```

## Notes

- See [`begin()`](begin.md) for what iteration actually yields (key names) and the
  important callout about not confusing them with `Stream` objects.
- `end()` equals `begin()` on a catalog with zero streams — the loop simply doesn't
  execute, rather than throwing.

## See Also

- [`Catalog::begin`](begin.md): the matching start-of-range iterator, with the full
  explanation of what iterating a `Catalog` yields.
- [`Catalog::nkeys`](nkeys.md): the direct way to get the key count without iterators.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="keywords.html">
    <span class="label">Next</span>
    <span class="title">keywords</span>
  </a>
</div>
