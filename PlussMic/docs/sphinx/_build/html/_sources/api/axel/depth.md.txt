<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e8678436b11 -->

# Axel::depth

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
std::int64_t Axel::depth() const
```

## What It Does

Returns the number of `Registry` frames stacked in this `Axel` — the
size of the depth axis.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of registries.

## Examples

Example 1. A three-frame `Axel` reports a depth of `3`.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
});
std::cout << axel.depth();
```

### Output

```text
3
```

Example 2. An undefined handle throws.

```cpp
pluss::scribble::Axel undef;
try {
  undef.depth();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel: undefined handle
```

## Notes

- [`Axel::size`](size.md) is a byte-identical alias for `depth()`.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::size`](size.md): the exact same value under a second name.
- [`Axel::empty`](empty.md): shorthand for `depth() == 0`.
- [`Axel::registry`](registry.md): fetch a single frame by index within
  `[0, depth())`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
