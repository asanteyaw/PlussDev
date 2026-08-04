<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7008f3bfdf96 -->

# Registry::to_kindle

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
pluss::kindle::Catalog Registry::to_kindle() const
```

## What It Does

Converts the registry into a `pluss::kindle::Catalog`, the homogeneous-columnar
counterpart to `Registry`'s heterogeneous, cell-by-cell model. Each field becomes
a `Catalog` stream. This is the explicit bridge every `save_*` method except
[`save_json()`](save_json.md) uses internally, and the one every other Registry
`save_*`/`is_stream_compatible`-adjacent important callout in this reference
points back to.

```{important} Requires every field to be stream-compatible
Every field must hold a single, uniform value kind. A registry with a genuinely
mixed-type field throws `"Registry::to_kindle: slot '<name>': slot is not
stream-compatible (kind=Mixed)"` — the surest way to know in advance whether a
given registry will convert cleanly is to try the conversion (or a `save_*`
call) inside a `try`/`catch`, since there is no public boolean pre-check for
this on `Registry` itself.
```

## Parameters

`None`

## Returns

`pluss::kindle::Catalog`
    A new catalog with one stream per field.

## Examples

Example 1. Convert a stream-compatible registry.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto ct = rg.to_kindle();
std::cout << "nkeys=" << ct.nkeys();
```

### Output

```text
nkeys=2
```

Example 2. A field with genuinely mixed value kinds throws.

```cpp
auto mixed = pluss::bundle::Registry({
    {"mixed", pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x")})},
});

try {
  auto ct = mixed.to_kindle();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::to_kindle: slot 'mixed': slot is not stream-compatible (kind=Mixed)
```

## Notes

- This is the exact check every `save_*` method except `save_json()` performs
  internally — see [`Registry::save_csv`](save_csv.md)'s important callout for
  the propagated error message.
- [`as_kindle()`](as_kindle.md) and [`kindelize()`](kindelize.md) are pure
  aliases for this method.
- [`Registry::from_kindle`](from_kindle.md) is the inverse conversion.

## See Also

- [`Registry::from_kindle`](from_kindle.md): the inverse conversion, from
  `Catalog` back to `Registry`.
- [`Registry::as_kindle`](as_kindle.md) / [`Registry::kindelize`](kindelize.md):
  pure aliases for this method.
- [`Registry::save_csv`](save_csv.md): one of several savers that route through
  this conversion.
- [`Slot::to_kindle`](../slot/to_kindle.md): the per-field conversion this
  method calls for each field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
