<!-- pluss-hand-authored -->

# Slot::tail

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::tail(std::int64_t n) const
```

## What It Does

Returns a new slot holding the last `n` values.

## Parameters

**n** (`std::int64_t`)
    The number of trailing values to keep. Clamped to `[0, size()]` —
    never throws for an out-of-range `n`.

## Returns

`Slot`
    A new slot with at most `n` values.

## Examples

Example 1. Take the last two values.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.tail(2);
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
size=2 vals=2,3
```

## Notes

- Clamps to `[0, size()]` — never throws for an out-of-range `n`.
- Implemented as `slice(size() - clamped_n, size())`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::head`](head.md): the mirror operation, keeping the first `n`
  values.
- [`Slot::slice`](slice.md): the general positional-range operation
  `tail()` is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
