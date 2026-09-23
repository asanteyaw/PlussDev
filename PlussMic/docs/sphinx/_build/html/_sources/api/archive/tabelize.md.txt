<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f74f0590efb6 -->

# Archive::tabelize

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::tabelize(const TabelizePolicy& policy = {}) const -> pluss::table::Atlas
```

## What It Does

Converts the whole archive into a `pluss::table::Atlas` — the depth-stacked table-stack
analog of `Archive`, the same way a single [`Catalog::tabelize()`](../catalog/tabelize.md)
produces one `DataFrame`. Each catalog becomes one `DataFrame` in the resulting `Atlas`, in
the same depth order, using the same `TabelizePolicy` (`align`/`align_by`/`fill`/`dtype`/
`collision`) documented on [`Catalog::tabelize`](../catalog/tabelize.md), applied
independently per catalog.

## Parameters

**policy** (`const TabelizePolicy&`, optional)
    The same alignment policy [`Catalog::tabelize()`](../catalog/tabelize.md) accepts —
    see that page for the full breakdown of each field.

## Returns

`pluss::table::Atlas`
    A depth-stacked `Atlas`, one `DataFrame` per catalog, in the archive's depth order.

## Examples

Example 1. Convert a two-catalog archive to an `Atlas`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Sunyani"})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"jan", "feb"})));

auto atlas = archive.tabelize();
std::cout << "atlas depth: " << atlas.depth() << "\n";
std::cout << atlas.dloc(0);
```

### Output

```text
atlas depth: 2
==================
     | id    city 
------------------
0    | 1     Accra
1    | 2     Tema 
------------------
[2 cs x 2 feat]
```

## Notes

- One `DataFrame` per catalog, same depth order as the receiver; `Atlas` exposes its own
  `depth()`/`dloc()`-style access, matching [`Archive::depth`](depth.md)/
  [`Archive::dloc`](dloc.md)'s own API shape.
- Each catalog is tabelized independently with the same `policy` — a ragged archive
  produces `DataFrame`s with different shapes, one per catalog.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Catalog::tabelize`](../catalog/tabelize.md): the single-catalog delegate this method
  applies to every catalog, and the full `TabelizePolicy` field reference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice_depth.html">
    <span class="label">Previous</span>
    <span class="title">slice_depth</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
