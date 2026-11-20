<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bd286dc1ef2e -->

# Catalog::stamp

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
std::uint64_t Catalog::stamp() const
```

## What It Does

Returns a cache token: a 64-bit number that identifies this stored data as of now, not what it contains. It changes whenever the contents can have changed (any of the streams was replaced or edited), so a copy of the catalog has the same stamp while a catalog rebuilt from equal values has its own.

It costs O(1): the data is never read. It is meant for caches that must notice edits cheaply, such as the join cache, which keys its reusable hashed right-hand sides on stamps. It is valid only while a copy of the catalog is kept alive, because it folds in the handle's address. To ask whether two catalogs hold the same data, use [`fingerprint()`](fingerprint.md).

## Parameters

`None`

## Returns

`std::uint64_t`
    A change token. Compare two stamps for equality; the number has no other meaning.

## Examples

Example 1. A copy shares the stamp; a catalog rebuilt from the same values does not.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog c1({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3}))});
auto copy = c1;
pluss::kindle::Catalog c2({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3}))});
std::cout << "copy stamp equal: " << (copy.stamp() == c1.stamp()) << "\n";
std::cout << "separately built, stamp equal: " << (c2.stamp() == c1.stamp());
```

### Output

```text
copy stamp equal: 1
separately built, stamp equal: 0
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="stats.html">
    <span class="label">Next</span>
    <span class="title">stats</span>
  </a>
</div>
