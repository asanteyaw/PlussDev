<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2969d40a1dd5 -->

# Registry::head

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::head(std::int64_t n) const
```

## What It Does

Returns a new registry keeping only the **first `n` values of every field**,
applying [`Slot::head(n)`](../slot/head.md) independently to each field.

```{important} Each field is clipped to its own length, independently
`Registry` fields are not required to share a common length (see
[`shape()`](shape.md)'s ragged-registry example). `head(n)` never throws for `n`
larger than a field's own size — that field's `Slot::head()` simply clamps to its
own length internally, so a shorter field just returns everything it has. On a
ragged registry, `head(n)` can leave fields with different sizes than each other.
```

## Parameters

**n** (`std::int64_t`)
    The number of leading values to keep per field. Values less than `0` are
    clamped to `0`.

## Returns

`Registry`
    A new registry with each field's `Slot` truncated to its own first `n` values
    (or fewer, if the field itself is shorter).

## Examples

Example 1. Keep the first two rows of a uniform registry.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3), pluss::make_series(4)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi"), pluss::make_note("Ho")})},
});

auto out = rg.head(2);
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).to_vec<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2]
```

Example 2. On a ragged registry, `n` exceeding a field's own length leaves that
field shorter than the others, rather than throwing.

```cpp
auto ragged = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

auto out = ragged.head(5);
auto sizes = out.sizes();
std::cout << "a: " << sizes["a"] << ", b: " << sizes["b"];
```

### Output

```text
a: 3, b: 2
```

## Notes

- Applies [`Slot::head()`](../slot/head.md) independently per field — see the
  important callout above for the ragged-registry consequence.
- `n < 0` is clamped to `0`, returning an all-fields-empty registry rather than
  throwing.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::tail`](tail.md): the mirror-image, last-`n`-values selection.
- [`Registry::slice`](slice.md): an arbitrary `[start, stop)` window, of which
  `head(n)` is a specialization (`slice(0, n)`).
- [`Catalog::head`](../catalog/head.md): the equivalent operation on the kindle
  model, where streams are typically expected to share a common length.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_any.html">
    <span class="label">Previous</span>
    <span class="title">has_any</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
