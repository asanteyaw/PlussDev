<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5245a91e3b16 -->

# Catalog::fingerprint

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
std::uint64_t Catalog::fingerprint() const
```

## What It Does

Returns a 64-bit hash of the catalog's contents: every key name together with the [`fingerprint()`](../stream/fingerprint.md) of its stream, in key order. Equal contents give equal fingerprints however the catalog was built, and different contents give different ones, up to the usual 64-bit collision probability.

The cost is one pass over the data. Use it to answer "did this data change?" or "are these two the same data?", for example to decide which downstream reports to rebuild. For a cheap token that notices edits without reading the data, see [`stamp()`](stamp.md).

## Parameters

`None`

## Returns

`std::uint64_t`
    A hash of the contents.

## Examples

Example 1. Equal contents have equal fingerprints, even when separately built; a changed value changes the fingerprint.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog c1({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});
pluss::kindle::Catalog c2({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});
pluss::kindle::Catalog c3({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Tamale"}))});
std::cout << "separately built, fingerprint equal: " << (c1.fingerprint() == c2.fingerprint()) << "\n";
std::cout << "one city changed, fingerprint equal: " << (c1.fingerprint() == c3.fingerprint());
```

### Output

```text
separately built, fingerprint equal: 1
one city changed, fingerprint equal: 0
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
