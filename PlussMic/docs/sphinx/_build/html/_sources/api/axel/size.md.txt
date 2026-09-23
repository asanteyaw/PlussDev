<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Axel::size

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
std::int64_t Axel::size() const
```

## What It Does

Returns the number of `Registry` frames stacked in this `Axel`.
Byte-identical to [`depth()`](depth.md) — `size()`'s body is a single
`return depth();`.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of registries — the same value `depth()` returns.

## Examples

Example 1. `size()` and `depth()` agree on a three-frame `Axel`.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
});
std::cout << axel.size() << " " << axel.depth();
```

### Output

```text
3 3
```

## Notes

- `size()` exists as a name-parity convenience with `Registry::size()`
  and `Slot::size()`, but for `Axel`, "size" means the depth axis (how
  many registries), not a row or value count — there is no separate
  concept of `Axel` row count, since each registry frame can have its
  own independent row shape.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `depth()` call).

## See Also

- [`Axel::depth`](depth.md): the canonical name for this same value.
- [`Axel::empty`](empty.md): shorthand for `size() == 0`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
