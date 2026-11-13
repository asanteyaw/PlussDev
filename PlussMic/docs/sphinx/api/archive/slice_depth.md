<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ba7e6c90ec2 -->

# Archive::slice_depth

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::slice_depth(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a new archive containing the contiguous half-open depth range `[start, stop)`.
Supports Python-style negative indices for both `start` and `stop`, and clamps out-of-range
bounds rather than throwing.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting depth-index. Negative values count from the end.

**stop** (`std::int64_t`)
    Exclusive ending depth-index. Negative values count from the end.

## Returns

`Archive`
    A new archive containing catalogs `[start, stop)`.

## Examples

Example 1. Slice a middle range of catalogs.

```cpp
using namespace pluss::kindle::literals;

std::vector<pluss::kindle::Catalog> cats;
for (int i = 0; i < 5; ++i) {
  cats.push_back(pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({double(i)})) }));
}
auto archive = pluss::kindle::Archive(cats);

auto out = archive.slice_depth(1, 4);
std::cout << "depth: " << out.depth() << "\n";
std::cout << "catalog(0):\n" << out.catalog(0);
```

### Output

```text
depth: 3
catalog(0):
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
```

Example 2. Negative `start`/`stop` count from the end, same convention as
[`catalog()`](catalog.md)'s negative indexing.

```cpp
using namespace pluss::kindle::literals;

std::vector<pluss::kindle::Catalog> cats;
for (int i = 0; i < 5; ++i) {
  cats.push_back(pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({double(i)})) }));
}
auto archive = pluss::kindle::Archive(cats);

auto out = archive.slice_depth(-3, -1);
std::cout << "depth: " << out.depth() << "\n";
std::cout << "catalog(0):\n" << out.catalog(0);
```

### Output

```text
depth: 2
catalog(0):
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [2]                                              
========================================================================================================================
```

`slice_depth(-3, -1)` on a 5-catalog archive is equivalent to `slice_depth(2, 4)` — depths
`2` and `3`.

## Notes

- `stop` is exclusive.
- Both `start` and `stop` support negative indexing, and both clamp into range rather than
  throwing on an out-of-bounds value.
- Returns a new `Archive`; the receiver is unmodified.

## See Also

- [`Archive::head`](head.md) / [`Archive::tail`](tail.md): fixed-count selections from
  either end, instead of an arbitrary range.
- [`Archive::catalog`](catalog.md): the single-catalog equivalent with the same negative-
  indexing convention.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
