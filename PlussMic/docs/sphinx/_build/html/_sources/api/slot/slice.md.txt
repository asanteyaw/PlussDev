<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60b2a6a755cd -->

# Slot::slice

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Returns a new slot holding the contiguous positional range `[start, stop)`.
Supports negative indices, counted from the end, the same way Python
slicing does.

## Parameters

**start** (`std::int64_t`)
    The inclusive start position. Negative values count from the end.

**stop** (`std::int64_t`)
    The exclusive stop position. Negative values count from the end.

## Returns

`Slot`
    A new slot with the values in `[start, stop)`.

## Examples

Example 1. A plain forward slice.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.slice(1, 3);
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
size=2 vals=2,3
```

Example 2. Negative indices count from the end.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.slice(-2, -1);
std::cout << "size=" << out.size() << " val="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0];
```

### Output

```text
size=1 val=2
```

## Notes

- Both bounds are clamped into `[0, size()]` after resolving negative
  indices — an out-of-range `start`/`stop` never throws, it just clips.
- If the resolved `stop` ends up before the resolved `start`, the result
  is an empty slot rather than throwing.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::head`](head.md) / [`Slot::tail`](tail.md): fixed-end shorthands
  built on `slice()`.
- [`Slot::take`](take.md): select values by arbitrary (non-contiguous)
  positions instead of a range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
