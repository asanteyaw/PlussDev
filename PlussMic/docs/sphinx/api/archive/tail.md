<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86c05ad48be9 -->

# Archive::tail

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::tail(std::int64_t n = 5) const
```

## What It Does

Returns a new archive containing the last `n` catalogs (depth-wise), defaulting to 5 when
omitted — the mirror of [`head()`](head.md). Same clamping behavior as `head()`:
requesting more than the archive's depth clamps to the whole archive, and `n <= 0` returns
an empty archive, rather than throwing.

## Parameters

**n** (`std::int64_t`, defaults to `5`)
    The number of trailing catalogs to keep.

## Returns

`Archive`
    A new archive containing at most the last `n` catalogs.

## Examples

Example 1. Take the last two catalogs of a multi-catalog archive.

```cpp
using namespace pluss::kindle::literals;

std::vector<pluss::kindle::Catalog> cats;
for (int i = 0; i < 7; ++i) {
  cats.push_back(pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({double(i)})) }));
}
auto archive = pluss::kindle::Archive(cats);

auto out = archive.tail(2);
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
id                       | Series       | Series[float64]  | 1       | [5]                                              
========================================================================================================================
```

The catalogs returned re-index from depth `0`, same as every other depth-selecting method
— `catalog(0)` here is the archive's original depth-index `5`, not `0`.

## Notes

- Defaults to `5` when `n` is omitted, unlike [`Stream::tail`](../stream/tail.md)/
  [`Catalog::tail`](../catalog/tail.md), which have no default.
- Clamps at both ends rather than throwing, same as [`head()`](head.md).
- Returns a new `Archive`; the receiver is unmodified.

## See Also

- [`Archive::head`](head.md): the mirror operation, taking the first `n` catalogs.
- [`Archive::slice_depth`](slice_depth.md): an arbitrary `[start, stop)` depth range instead
  of a fixed count from the back.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
