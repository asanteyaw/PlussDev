<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e82e41676db0 -->

# Atlas::tabelize

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
static Atlas Atlas::tabelize(const pluss::kindle::Archive& archive)
static Atlas Atlas::tabelize(const pluss::kindle::Archive& archive, const pluss::kindle::TabelizePolicy& policy)
```

## What It Does

**A `static` factory method** — call it as `Atlas::tabelize(...)`, not
`atlas.tabelize(...)`. Converts an `Archive` into an `Atlas`, one frame per catalog, in the
same depth order — the inverse of [`kindelize()`](kindelize.md), which goes the other
direction (`Atlas` → `Archive`). Internally, each catalog is converted via
[`Catalog::tabelize()`](../catalog/tabelize.md), using the same `policy` for every one, and
the archive's own depth labels are carried over.

## Parameters

**archive** (`const pluss::kindle::Archive&`)
    The source archive. Must be defined.

**policy** (`const pluss::kindle::TabelizePolicy&`, optional)
    The same alignment policy [`Catalog::tabelize()`](../catalog/tabelize.md) accepts,
    applied uniformly to every catalog.

## Returns

`Atlas`
    A new atlas with one frame per catalog in `archive`, carrying over the archive's own
    depth labels.

## Examples

Example 1. Convert a two-catalog archive into an `Atlas`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})) });
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"jan", "feb"})));

auto atlas = pluss::table::Atlas::tabelize(archive);
std::cout << "depth: " << atlas.depth() << "\n";
std::cout << "depth labels carried over: " << atlas.label().has() << "\n";
std::cout << atlas.dloc(0);
```

### Output

```text
depth: 2
depth labels carried over: 1
===========
     | id  
-----------
0    | 1   
1    | 2   
-----------
[2 cs x 1 feat]
```

## Notes

- A `static` factory — call as `Atlas::tabelize(archive)`, not on an existing instance.
- The archive's own depth labels are carried over onto the resulting atlas's
  [`label()`](label.md).
- `policy` is applied uniformly to every catalog via
  [`Catalog::tabelize()`](../catalog/tabelize.md) — see that page for the full field
  breakdown.

## See Also

- [`Atlas::kindelize`](kindelize.md): the inverse conversion, from `Atlas` back to
  `Archive`.
- [`Archive::tabelize`](../archive/tabelize.md): the depth-stacked-catalog analog of this
  method, producing this same `Atlas` type directly from an instance method call.
- [`Catalog::tabelize`](../catalog/tabelize.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="squeeze.html">
    <span class="label">Previous</span>
    <span class="title">squeeze</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
