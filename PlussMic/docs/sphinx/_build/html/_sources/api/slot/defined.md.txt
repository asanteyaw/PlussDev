<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Slot::defined

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
bool Slot::defined() const noexcept
```

## What It Does

Reports whether the `Slot` handle owns a concrete, usable payload. A
default-constructed `Slot` is undefined; every other constructor produces a
defined one.

```{important} Almost every other method requires defined() to be true
Calling `size()`, `values()`, `at()`, or nearly any other method on an
undefined `Slot` throws `"Slot: undefined handle"`. `defined()` (and its
sibling [`unique()`](unique.md)) are the two exceptions — both are
`noexcept` and safe to call on any `Slot`, defined or not.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the slot owns a payload; `false` for a default-constructed
    `Slot`.

## Examples

Example 1. A slot built from values is defined.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)});
std::cout << std::boolalpha << slot.defined();
```

### Output

```text
true
```

Example 2. A default-constructed slot is not.

```cpp
pluss::scribble::Slot slot;
std::cout << std::boolalpha << slot.defined();
```

### Output

```text
false
```

## Notes

- See the important callout above — this is the one check safe to make
  before calling anything else on a `Slot` that might be undefined.
- `noexcept` — never throws, regardless of state.

## See Also

- [`Slot::unique`](unique.md): the other `noexcept` state check, reporting
  reference-count uniqueness rather than payload presence.
- [`Slot::empty`](empty.md): a defined slot can still be empty (zero
  values) — a different condition from being undefined.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat.html">
    <span class="label">Previous</span>
    <span class="title">concat</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
