<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 77488733cfce -->

# Axel::head

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::head(std::int64_t n) const
```

## What It Does

Returns the first `n` registry frames (depth-wise), clamped to
`[0, depth()]`. Equivalent to `slice(0, n)` after clamping.

## Parameters

**n** (`std::int64_t`)
    The number of leading frames to keep. Values greater than `depth()`
    are clamped down to `depth()`; negative values are clamped up to
    `0`.

## Returns

`Axel`
    A new axel holding the first (clamped) `n` frames. If
    [`labels()`](labels.md) is engaged, the label `Slot` is sliced
    alongside the frames.

## Examples

Example 1. Keep the first two frames of a five-frame `Axel`.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(4)})}}),
});
auto out = axel.head(2);
std::cout << "depth=" << out.depth() << " last_id="
          << std::get<pluss::Series>(out.registry(out.depth() - 1).fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
depth=2 last_id=1
```

Example 2. Asking for more frames than exist clamps to `depth()`
instead of throwing.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
});
auto out = axel.head(100);
std::cout << out.depth();
```

### Output

```text
2
```

## Notes

- `n` is silently clamped, never throws for an out-of-range value.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `slice()` call).

## See Also

- [`Axel::tail`](tail.md): the same clamped-count idiom, from the end.
- [`Axel::slice`](slice.md): the general-purpose range selector `head()`
  delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
