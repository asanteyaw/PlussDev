<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a0d3777268d8 -->

# Catalog::begin

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
const_iterator Catalog::begin() const
```

## What It Does

Returns an iterator to the first **key name** in the catalog — `const_iterator` is
`std::vector<std::string>::const_iterator`, so `begin()`/[`end()`](end.md) make `Catalog`
directly usable in range-`for` loops and with standard algorithms (`std::find`,
`std::distance`, etc.). Iteration yields the stream *names*, in the catalog's own key
order — not the streams themselves and not row values.

```{important} Iterates key names, not streams or row values
`for (const auto& x : catalog)` binds `x` to each `std::string` key name, not to a
`Stream`. If you want the actual stream for a key, look it up with
[`keys(key)`](keys.md)/[`get(key)`](get.md) inside the loop.
```

## Parameters

`None`

## Returns

`const_iterator`
    An iterator to the first key name, or equal to `end()` if the catalog has no streams.

## Examples

Example 1. Range-`for` over a catalog yields its key names, in order.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : ct) {
  std::cout << key << " ";
}
```

### Output

```text
id metric city 
```

Example 2. Because `begin()`/`end()` return standard iterators, `Catalog` works directly
with `<algorithm>` functions — here, `std::find` to check whether a key exists by name.

```cpp
#include <algorithm>

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto found = std::find(ct.begin(), ct.end(), "metric");
std::cout << std::boolalpha << (found != ct.end());
```

### Output

```text
true
```

## Notes

- Iterates key **names** (`std::string`), not `Stream` objects or row values — see the
  important callout above.
- `Catalog::const_iterator` is a plain `std::vector<std::string>::const_iterator`, so any
  standard algorithm that accepts forward iterators works directly.
- For an existing, more direct way to enumerate names without iterators, see
  [`keys()`](keys.md), which returns the same underlying vector by reference.

## See Also

- [`Catalog::end`](end.md): the matching past-the-end iterator.
- [`Catalog::keys`](keys.md): the underlying `std::vector<std::string>` this iterates over,
  accessible directly without iterators.
- [`Catalog::keys`](keys.md) (`keys(key)`) / [`Catalog::get`](get.md): fetch the actual `Stream` for a key
  name found during iteration.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html">
    <span class="label">Previous</span>
    <span class="title">at</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
