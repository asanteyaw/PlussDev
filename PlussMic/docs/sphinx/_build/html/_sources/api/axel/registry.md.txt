<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b722d6b4bcb8 -->

# Axel::registry

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Registry Axel::registry(std::int64_t i) const
```

## What It Does

Returns the `Registry` frame at depth index `i`.

## Parameters

**i** (`std::int64_t`)
    The depth index to fetch, in `[0, depth())`. Out-of-range values
    throw — there is no negative-index wraparound, unlike
    [`slice()`](slice.md).

## Returns

`Registry`
    The registry frame at index `i`.

## Examples

Example 1. Fetch the second frame (index `1`) from a three-frame
`Axel`.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(3)})}}),
});
auto reg = axel.registry(1);
std::cout << std::get<pluss::Series>(reg.fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
2
```

Example 2. An out-of-range index throws.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
});
try {
  axel.registry(3);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Axel::registry: index out of bounds
```

## Notes

- `i` must satisfy `0 <= i < depth()`; unlike [`slice()`](slice.md),
  negative indices are **not** interpreted as "from the end" here — any
  negative `i` is out of bounds and throws.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::registries`](registries.md): fetch every frame at once as a
  `const std::vector<Registry>&`.
- [`Axel::depth`](depth.md): the valid upper bound for `i`.
- [`Axel::slice`](slice.md): select a contiguous *range* of frames
  (with Python-style negative-index support) instead of one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="registries.html">
    <span class="label">Previous</span>
    <span class="title">registries</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
