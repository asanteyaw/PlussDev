<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c55e37315c87 -->

# Catalog::scribbelize

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
pluss::scribble::Registry Catalog::scribbelize() const
```

## What It Does

Converts the catalog into a `pluss::scribble::Registry` — the reverse direction of
[`Registry::kindelize`](../registry/kindelize.md). Every stream the catalog holds is
converted to a `pluss::scribble::Slot` (via [`Stream::scribbelize`](../stream/scribbelize.md))
and reassembled into a registry field, in exactly the catalog's own
[`keys()`](keys.md) order. Nothing about the values themselves changes shape or
count — only the model they live in: a catalog is a named collection of typed,
columnar `Stream`s, while a registry is a named collection of `Slot`s, each a
heterogeneous, per-element `std::variant` sequence. The two models are designed to
round-trip: `catalog.scribbelize().kindelize()` reproduces the same key set (though not
necessarily byte-identical internals — see the important callout below).

```{important} scribbelize() is per-stream conversion, not a single bulk pass
Catalog::scribbelize() does not touch the catalog's underlying storage directly -- it
calls keys(key).scribbelize() once per key, which itself dispatches on that stream's own
kind() (Series/Note/Timeline/TimeIndex) and, within Series, further dispatches on
scalar_type() (Bool/Int64/other, the last cast through Float64). A catalog with a
Series-typed key and a Note-typed key is converted through two different code paths
internally, even though this method's own signature looks uniform. See
Stream::scribbelize's own page for the full per-kind conversion table.
```

## Parameters

`None`

## Returns

`pluss::scribble::Registry`
    A new registry with one `Slot` field per catalog key, in catalog key order.

## Examples

Example 1. A three-key catalog (`Series` numeric, `Series` float, `Note` text) —
`scribbelize()` produces a registry with the same field count and key order.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto reg = ct.scribbelize();
std::cout << reg.nfields() << "\n";
std::cout << reg.fields().at(0) << "," << reg.fields().at(1) << "," << reg.fields().at(2) << "\n";
std::cout << reg.fields("id").size() << "\n";
```

### Output

```text
3
id,metric,city
3
```

Example 2 (round trip, not a distinct rendering). Converting back with
[`Registry::kindelize`](../registry/kindelize.md) reproduces the same key count —
confirming the two directions are genuinely inverse operations for this data.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});
auto reg = ct.scribbelize();
auto ct2 = reg.kindelize();
std::cout << ct2.nkeys() << "\n";
```

### Output

```text
2
```

Example 3 (edge case). An undefined catalog throws before any key is touched.

```cpp
try {
  pluss::kindle::Catalog undefined_catalog;
  undefined_catalog.scribbelize();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Catalog::scribbelize: catalog undefined
```

## Notes

- Requires `defined()` — an undefined catalog throws
  `"Catalog::scribbelize: catalog undefined"` immediately, before any key is visited.
- Field order in the resulting `Registry` always matches
  [`Catalog::keys()`](keys.md)'s own order — `scribbelize()` performs no reordering,
  sorting, or deduplication of its own.
- An empty catalog (zero keys, but still `defined()`) produces a registry with zero
  fields, not a throw.
- Every value in every stream — including null positions, when a stream carries a
  validity mask — round-trips through the per-stream conversion; see
  [`Stream::scribbelize`](../stream/scribbelize.md)'s own page for exactly how nulls are
  represented on the `Slot` side (`std::monostate`).

## See Also

- [`Registry::kindelize`](../registry/kindelize.md): the reverse bridge, from the
  scribble model back into Kindle.
- [`Stream::scribbelize`](../stream/scribbelize.md): what this method calls once per key —
  read that page for the full per-`StreamKind` conversion behavior.
- [`Archive::scribbelize`](../archive/scribbelize.md): the depth-stacked sibling of this
  method, converting a whole `Archive` (a stack of catalogs) into an `Axel`.
- [`Catalog::keys`](keys.md): what determines the field order this method preserves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
