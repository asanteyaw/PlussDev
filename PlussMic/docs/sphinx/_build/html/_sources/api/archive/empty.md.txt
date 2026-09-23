<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Archive::empty

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::empty() const -> bool
```

## What It Does

Reports whether the archive has zero depth — shorthand for `depth() == 0`. A distinct check
from [`defined()`](defined.md): an archive can be defined (has a real, valid payload) and
still be empty (zero catalogs stacked in it).

## Parameters

`None`

## Returns

`bool`
    `true` if the archive has zero catalogs; `false` otherwise.

## Examples

Example 1. A non-empty archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto archive = pluss::kindle::Archive({c0});

std::cout << archive.empty();
```

### Output

```text
0
```

Example 2. An archive built with zero catalogs is empty, but still defined.

```cpp
auto archive = pluss::kindle::Archive(std::vector<pluss::kindle::Catalog>{});

std::cout << "defined: " << archive.defined() << "\n";
std::cout << "empty: " << archive.empty() << "\n";
std::cout << "depth: " << archive.depth();
```

### Output

```text
defined: 1
empty: 1
depth: 0
```

## Notes

- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.
- Distinct from `defined()` — see Example 2: empty and defined are independent axes.

## See Also

- [`Archive::depth`](depth.md) / [`Archive::size`](size.md): the depth this method checks
  against zero.
- [`Archive::defined`](defined.md): whether the archive has a payload at all, independent
  of depth.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
