<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3041f4c7bf86 -->

# Slot::head

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::head(std::int64_t n = 5) const
```

## What It Does

Returns a new slot holding the first `n` values.

## Parameters

**n** (`std::int64_t`, default `5`)

The number of leading values to keep. Clamped to `[0, size()]` — never
throws for an out-of-range `n`.

## Returns

`Slot`
    A new slot with at most `n` values.

## Examples

Example 1. Take the first two values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.head(2);
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
size=2 vals=1,2
```

Example 2. Requesting more than `size()` clamps rather than throwing.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.head(100);
std::cout << out.size();
```

### Output

```text
3
```

Example 3. Call `head()` with no argument on a slot of seven cells, Series and Note mixed; `n` defaults to 5.

```cpp
auto slot = pluss::scribble::Slot({
    pluss::make_series(1), pluss::make_note("two"), pluss::make_series(3), pluss::make_note("four"),
    pluss::make_series(5), pluss::make_note("six"), pluss::make_series(7)});

std::cout << slot.head();
```

### Output

```text
0  1
1  two
2  3
3  four
4  5
ptype: Mixed (Series, Note)
```

The first five cells come back with their kinds unchanged, so the footer still reads Mixed.

## Notes

- Clamps to `[0, size()]` — never throws for an out-of-range `n`, unlike
  [`slice()`](slice.md)/[`take()`](take.md).
- Implemented as `slice(0, clamped_n)`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::tail`](tail.md): the mirror operation, keeping the last `n`
  values.
- [`Slot::slice`](slice.md): the general positional-range operation
  `head()` is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
