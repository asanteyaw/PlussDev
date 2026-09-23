<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23179561f0c5 -->

# Axel::slice

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Selects a contiguous range of registry frames `[start, stop)` along the
depth axis, with Python-style negative-index support and clamping — the
range is never out of bounds, it's silently narrowed instead.

## Parameters

**start** (`std::int64_t`)
    The first frame index to include. Negative values count from the
    end (`-1` is the last frame). Clamped into `[0, depth()]` after
    resolving negatives.

**stop** (`std::int64_t`)
    One past the last frame index to include. Same negative-index and
    clamping rules as `start`. If the resolved `stop` ends up before the
    resolved `start`, the result is an empty (zero-depth) `Axel`, not a
    throw.

## Returns

`Axel`
    A new axel holding frames `[start, stop)`. If [`labels()`](labels.md)
    is engaged, the label `Slot` is sliced alongside the frames via
    `Slot::slice(s, e)`.

## Examples

Example 1. Slice out the middle frames with positive indices.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(4)})}}),
});
auto out = axel.slice(1, 3);
std::cout << "depth=" << out.depth() << " first_id="
          << std::get<pluss::Series>(out.registry(0).fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
depth=2 first_id=1
```

Example 2. Negative indices count from the end, same as Python slicing.

```cpp
auto axel = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(4)})}}),
});
auto out = axel.slice(-2, 5);
std::cout << "depth=" << out.depth() << " first_id="
          << std::get<pluss::Series>(out.registry(0).fields("ids").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
depth=2 first_id=3
```

`-2` resolves to `depth() - 2 = 3`, so this keeps frames `3` and `4`.

Example 3. Labels are sliced in lockstep with the frames they describe.

```cpp
auto labeled = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
}, pluss::scribble::Slot({pluss::make_note("l0"), pluss::make_note("l1"), pluss::make_note("l2")}));

auto out = labeled.slice(1, 3);
std::cout << std::get<pluss::Note>(out.labels()->at(0)).at(0);
```

### Output

```text
l1
```

## Notes

- Unlike [`registry(i)`](registry.md), `slice()` supports Python-style
  negative indices for both `start` and `stop`.
- Both indices are clamped, so no combination of `start`/`stop` throws —
  a `stop` before an effective `start` simply yields a zero-depth
  result.
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.

## See Also

- [`Axel::head`](head.md), [`Axel::tail`](tail.md): convenience
  wrappers around `slice()` for the two most common ranges.
- [`Axel::registry`](registry.md): fetch a single frame by index
  instead of a range (with strict, non-negative bounds checking).
- [`Axel::labels`](labels.md): the label slot that stays aligned with
  the sliced frames.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
