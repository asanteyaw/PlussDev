<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69e9a946d502 -->

# Slot::tail

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::tail(std::int64_t n = 5) const
```

## What It Does

Returns a new slot holding the last `n` values.

## Parameters

**n** (`std::int64_t`, default `5`)

The number of trailing values to keep. Clamped to `[0, size()]` —
never throws for an out-of-range `n`.

## Returns

`Slot`
    A new slot with at most `n` values.

## Examples

Example 1. Take the last two values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.tail(2);
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
size=2 vals=2,3
```

Example 2. Call `tail()` with no argument on a slot of seven cells, Series and Note mixed; `n` defaults to 5.

```cpp
auto slot = pluss::scribble::Slot({
    pluss::make_series(1), pluss::make_note("two"), pluss::make_series(3), pluss::make_note("four"),
    pluss::make_series(5), pluss::make_note("six"), pluss::make_series(7)});

std::cout << slot.tail();
```

### Output

```text
0  3
1  four
2  5
3  six
4  7
ptype: Mixed (Series, Note)
```

The last five cells come back with their kinds unchanged, so the footer still reads Mixed.

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
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
