<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 92848f5e6b98 -->

# Axel::head

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::head(std::int64_t n = 5) const
```

## What It Does

Returns the first `n` registry frames (depth-wise), clamped to
`[0, depth()]`. Equivalent to `slice(0, n)` after clamping.

## Parameters

**n** (`std::int64_t`, default `5`)

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
          << std::get<pluss::Series>(out.registry(out.depth() - 1).fields("ids").at(0)).host<std::int64_t>()[0];
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

Example 3. Call `head()` with no argument on a axel of seven registries; `n` defaults to 5.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(4)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(5)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(6)})}}),
});

auto out = axel.head();
std::cout << "depth " << out.depth() << "\n";
for (std::int64_t i = 0; i < out.depth(); ++i) std::cout << out.registry(i).fields("ids") << "\n";
```

### Output

```text
depth 5
0  0
ptype: Series

0  1
ptype: Series

0  2
ptype: Series

0  3
ptype: Series

0  4
ptype: Series
```

With no argument the axel keeps its first five registries, so the depth is 5 and the ids run 0 to 4.

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
