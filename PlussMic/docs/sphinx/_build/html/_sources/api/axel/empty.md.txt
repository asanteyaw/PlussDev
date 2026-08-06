<!-- pluss-hand-authored -->

# Axel::empty

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
bool Axel::empty() const
```

## What It Does

Reports whether this `Axel` has zero depth (no registry frames at all).
Equivalent to `depth() == 0`.

## Parameters

`None`

## Returns

`bool`
    `true` if the axel has no registry frames; `false` otherwise.

## Examples

Example 1. A three-frame `Axel` is not empty; one built from an empty
registry vector is.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
});
auto empty_axel = pluss::bundle::Axel(std::vector<pluss::bundle::Registry>{});
std::cout << std::boolalpha << axel.empty() << " " << empty_axel.empty();
```

### Output

```text
false true
```

## Notes

- `empty()` is about depth (registry-frame count), not about whether
  any individual registry inside has rows or fields — a one-frame
  `Axel` wrapping a registry with zero rows is still *not* `empty()`.
- An `Axel` with zero depth is still `defined()` (see
  [`defined()`](defined.md)'s Example 1) — `empty()` and `defined()`
  answer different questions.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `depth()` call).

## See Also

- [`Axel::depth`](depth.md) / [`size()`](size.md): the underlying count
  `empty()` compares against zero.
- [`Axel::defined`](defined.md): whether the handle is bound to storage
  at all, independent of depth.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="fieldaxis.html">
    <span class="label">Next</span>
    <span class="title">fieldaxis</span>
  </a>
</div>
