<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 517d3517dcd7 -->

# Archive::head

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::head(std::int64_t n = 5) const -> Archive
```

## What It Does

Returns a new archive containing the first `n` catalogs (depth-wise), defaulting to 5 when
omitted. Requesting more than the archive's depth clamps to the whole archive rather than
throwing; `n <= 0` returns an empty (zero-depth) archive rather than throwing.

## Parameters

**n** (`std::int64_t`, defaults to `5`)
    The number of leading catalogs to keep.

## Returns

`Archive`
    A new archive containing at most the first `n` catalogs.

## Examples

Example 1. Take the first catalog of a multi-catalog archive.

```cpp
using namespace pluss::kindle::literals;

std::vector<pluss::kindle::Catalog> cats;
for (int i = 0; i < 7; ++i) {
  cats.push_back(pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({double(i)})) }));
}
auto archive = pluss::kindle::Archive(cats);

auto out = archive.head(3);
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
id                       | Series       | Series[float64]  | 1       | [0]                                              
========================================================================================================================
```

Example 2. The default (`n = 5`) applies when omitted; out-of-range values clamp rather
than throw.

```cpp
using namespace pluss::kindle::literals;

std::vector<pluss::kindle::Catalog> cats;
for (int i = 0; i < 7; ++i) {
  cats.push_back(pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({double(i)})) }));
}
auto archive = pluss::kindle::Archive(cats);

std::cout << "head() depth: " << archive.head().depth() << "\n";
std::cout << "head(100) depth: " << archive.head(100).depth() << "\n";
std::cout << "head(0) depth: " << archive.head(0).depth();
```

### Output

```text
head() depth: 5
head(100) depth: 7
head(0) depth: 0
```

## Notes

- Defaults to `5` when `n` is omitted, unlike [`Stream::head`](../stream/head.md)/
  [`Catalog::head`](../catalog/head.md), which have no default.
- Clamps at both ends rather than throwing — see Example 2.
- Returns a new `Archive`; the receiver is unmodified.

## See Also

- [`Archive::tail`](tail.md): the mirror operation, taking the last `n` catalogs.
- [`Archive::slice_depth`](slice_depth.md): an arbitrary `[start, stop)` depth range instead
  of a fixed count from the front.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
