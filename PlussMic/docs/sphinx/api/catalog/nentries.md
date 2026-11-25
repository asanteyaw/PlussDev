<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ed79435661f -->

# Catalog::nentries

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
std::int64_t Catalog::nentries() const
```

## What It Does

Returns the number of entries a catalog holds when **every key has the same number of entries**. An *entry* is one position along a key's stream: entry 0 of `id` and entry 0 of `city` are the first values of their streams, and a catalog whose keys all agree on their entry count is aligned.

Because a catalog may be ragged, `nentries()` is the strict counterpart of [`minentries()`](minentries.md) and [`maxentries()`](maxentries.md): it throws instead of choosing between different lengths. A catalog with no keys has zero entries.

## Parameters

None.

## Returns

`std::int64_t`
    The shared entry count; `0` for a catalog with no keys.

## Examples

Example 1. An aligned catalog: every key has three entries.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << ct.nentries();
```

### Output

```text
3
```

Example 2. A ragged catalog: the keys disagree, so `nentries()` throws and names the alternatives.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1.0, 0.0})),
});

try {
    std::cout << ct.nentries();
} catch (const pluss::Error& e) {
    std::cout << e.message();
}
```

### Output

```text
Catalog::nentries: catalog has ragged stream lengths; use minentries()/maxentries() or tabelize()
```

Example 3. A catalog with no keys has no entries.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct(std::initializer_list<pluss::kindle::NamedStream>{});

std::cout << ct.nentries();
```

### Output

```text
0
```

Example 4. An undefined catalog has no handle to count.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct;

try {
    std::cout << ct.nentries();
} catch (const pluss::Error& e) {
    std::cout << e.message();
}
```

### Output

```text
Catalog: undefined handle
```


## Notes

- Throws when the keys disagree. Use [`minentries()`](minentries.md) or [`maxentries()`](maxentries.md) on a ragged catalog, or [`tabelize()`](tabelize.md) to pad it to a rectangle first.
- Counts entries, not keys. [`nkeys()`](nkeys.md) is the number of keys.
- Throws `"Catalog: undefined handle"` on a default-constructed catalog (see [`defined()`](defined.md)).

## See Also

- [`Catalog::minentries`](minentries.md) / [`Catalog::maxentries`](maxentries.md): the shortest and longest key, for ragged catalogs.
- [`Catalog::nkeys`](nkeys.md): the number of keys.
- [`Catalog::shape`](shape.md): `{maxentries(), nkeys()}` in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="nkeys.html">
    <span class="label">Next</span>
    <span class="title">nkeys</span>
  </a>
</div>
