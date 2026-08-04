<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Registry::empty

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry::empty() const -> bool
```

## What It Does

Reports whether the registry has **zero fields** — `nfields() == 0`. This is a
question about the registry's *width* (how many named slots it holds), not about
whether any individual slot has zero values.

```{important} A registry with slots that each have zero values is not empty()
`empty()` only looks at `nfields()`. A registry holding one or more slots, each of
which happens to have zero elements, still reports `empty() == false` — it has
slots, they're just short. See Example 2.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the registry has no named fields; `false` otherwise. Requires a
    defined registry; throws on an undefined one.

## Examples

Example 1. A registry with fields is not empty.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
});

std::cout << std::boolalpha << rg.empty();
```

### Output

```text
false
```

Example 2. A slot with zero values still counts as a field — the registry is not
`empty()`, even though the slot itself is short.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot(std::vector<pluss::bundle::Slot::Value>{})},
});

std::cout << std::boolalpha << rg.empty() << "\n";
std::cout << rg.nfields();
```

### Output

```text
false
1
```

Example 3. An explicitly empty field list (zero fields) is `empty()`.

```cpp
pluss::bundle::Registry rg(std::vector<std::pair<std::string, pluss::bundle::Slot>>{});
std::cout << std::boolalpha << rg.empty();
```

### Output

```text
true
```

## Notes

- Equivalent to `nfields() == 0` — a question about field count, not value count.
  See the important callout above.
- Throws `"Registry: undefined handle"` on an undefined registry — see
  [`defined()`](defined.md).

## See Also

- [`Registry::nfields`](nfields.md): the value `empty()` compares against zero.
- [`Registry::defined`](defined.md): a different, prerequisite question — whether
  the registry has a backing handle at all.
- [`Catalog::empty`](../catalog/empty.md): the equivalent check on the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="end.html">
    <span class="label">Next</span>
    <span class="title">end</span>
  </a>
</div>
