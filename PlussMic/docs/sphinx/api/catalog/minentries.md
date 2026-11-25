<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4af76cac08b4 -->

# Catalog::minentries

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
std::int64_t Catalog::minentries() const
```

## What It Does

Returns the entry count of the **shortest** key. On an aligned catalog it equals [`nentries()`](nentries.md); on a ragged one it is the number of entries every key is guaranteed to have, which makes it the safe bound for any loop that reads all the keys by position.

A catalog with no keys has a minimum of zero.

## Parameters

None.

## Returns

`std::int64_t`
    The smallest key length; `0` for a catalog with no keys.

## Examples

Example 1. A ragged catalog: the shortest key sets the minimum.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1.0, 0.0})),
});

std::cout << ct.minentries();
```

### Output

```text
2
```

Example 2. On an aligned catalog the minimum is the shared count.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << ct.minentries() << " " << ct.nentries();
```

### Output

```text
3 3
```

Example 3. A catalog with no keys has a minimum of zero.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct(std::initializer_list<pluss::kindle::NamedStream>{});

std::cout << ct.minentries();
```

### Output

```text
0
```

Example 4. An undefined catalog has no handle to measure.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct;

try {
    std::cout << ct.minentries();
} catch (const pluss::Error& e) {
    std::cout << e.message();
}
```

### Output

```text
Catalog: undefined handle
```


## Notes

- Never throws on a ragged catalog, unlike [`nentries()`](nentries.md).
- Throws `"Catalog: undefined handle"` on a default-constructed catalog.

## See Also

- [`Catalog::maxentries`](maxentries.md): the longest key.
- [`Catalog::nentries`](nentries.md): the shared count, for aligned catalogs only.
- [`Catalog::eloc`](eloc.md): select entries by position across every key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
