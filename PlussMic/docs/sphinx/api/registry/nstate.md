<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 959f74fe10d9 -->

# Registry::nstate

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::int64_t Registry::nstate() const
```

## What It Does

Returns the registry's shared row count, **requiring** every field to have the
same length — the strict counterpart to [`minstate()`](minstate.md)/[`maxstate()`](maxstate.md),
which never throw on a ragged registry.

```{important} Throws on a ragged registry
Unlike every row-selection method on `Registry` (`head()`, `tail()`, `slice()`,
`take()`), which tolerate fields of differing lengths, `nstate()` throws
`"Registry::nstate: registry has ragged slot lengths; use minstate()/maxstate() or shape()"`
the moment any field's length disagrees with the first field's. Use it when you
specifically want to assert alignment, not merely inspect size.
```

## Parameters

`None`

## Returns

`std::int64_t`
    The common field length; `0` for an empty registry.

## Examples

Example 1. A registry whose fields all share the same length.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

std::cout << rg.nstate();
```

### Output

```text
3
```

Example 2. A ragged registry (fields of different lengths) throws.

```cpp
auto ragged = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

try {
  auto n = ragged.nstate();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::nstate: registry has ragged slot lengths; use minstate()/maxstate() or shape()
```

Example 3. An explicitly empty registry reports `0`, not a throw.

```cpp
pluss::bundle::Registry rg(std::vector<std::pair<std::string, pluss::bundle::Slot>>{});
std::cout << rg.nstate();
```

### Output

```text
0
```

## Notes

- Requires field-length alignment — see the important callout above. This is the
  same requirement [`sample()`](sample.md) enforces internally.
- An empty registry (`empty() == true`) reports `0` rather than throwing.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::minstate`](minstate.md) / [`Registry::maxstate`](maxstate.md): the
  non-throwing, ragged-safe alternatives.
- [`Registry::shape`](shape.md): pairs `maxstate()` with the field count in one
  call, also ragged-safe.
- [`Registry::sample`](sample.md): the row-selection method that shares this
  same alignment requirement.
- [`Catalog::nrows`](../catalog/nrows.md): the equivalent strict row-count check
  on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="nt.html">
    <span class="label">Next</span>
    <span class="title">nt</span>
  </a>
</div>
