<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1dceb4414975 -->

# Registry::minstate

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::int64_t Registry::minstate() const
```

## What It Does

Returns the **shortest** field's length in the registry — never throws, even on a
ragged registry (fields of differing lengths), unlike [`nstate()`](nstate.md).

## Parameters

`None`

## Returns

`std::int64_t`
    The shortest field's value count; `0` for an empty registry.

## Examples

Example 1. A ragged registry reports its shortest field's length.

```cpp
auto rg = pluss::scribble::Registry({
    {"a", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::scribble::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

std::cout << rg.minstate();
```

### Output

```text
2
```

Example 2. An explicitly empty registry reports `0`.

```cpp
pluss::scribble::Registry rg(std::vector<std::pair<std::string, pluss::scribble::Slot>>{});
std::cout << rg.minstate();
```

### Output

```text
0
```

## Notes

- Never throws, regardless of field-length agreement — contrast with
  [`nstate()`](nstate.md), which requires it.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::maxstate`](maxstate.md): the longest-field counterpart.
- [`Registry::nstate`](nstate.md): the strict, throwing form that requires every
  field to share this same length.
- [`Registry::sizes`](sizes.md): every field's individual length, rather than
  just the minimum.
- [`Catalog::minrows`](../catalog/minrows.md): the equivalent check on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maxstate.html">
    <span class="label">Previous</span>
    <span class="title">maxstate</span>
  </a>
  <a class="next-link" href="morph.html">
    <span class="label">Next</span>
    <span class="title">morph</span>
  </a>
</div>
