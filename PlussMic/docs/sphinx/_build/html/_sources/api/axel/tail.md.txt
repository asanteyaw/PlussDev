<!-- pluss-hand-authored -->

# Axel::tail

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::tail(std::int64_t n) const
```

## What It Does

Returns the last `n` registry frames (depth-wise), clamped to
`[0, depth()]`. Equivalent to `slice(depth() - n, depth())` after
clamping.

## Parameters

**n** (`std::int64_t`)
    The number of trailing frames to keep. Values greater than
    `depth()` are clamped down to `depth()`; negative values are
    clamped up to `0`.

## Returns

`Axel`
    A new axel holding the last (clamped) `n` frames. If
    [`labels()`](labels.md) is engaged, the label `Slot` is sliced
    alongside the frames.

## Examples

Example 1. Keep the last two frames of a five-frame `Axel`.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(0)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(3)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(4)})}}),
});
auto out = axel.tail(2);
std::cout << "depth=" << out.depth() << " first_id="
          << std::get<pluss::Series>(out.registry(0).fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
depth=2 first_id=3
```

## Notes

- `n` is silently clamped, never throws for an out-of-range value.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"` (via the underlying `slice()` call).

## See Also

- [`Axel::head`](head.md): the same clamped-count idiom, from the
  start.
- [`Axel::slice`](slice.md): the general-purpose range selector
  `tail()` delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
