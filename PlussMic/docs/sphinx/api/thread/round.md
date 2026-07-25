<!-- pluss-hand-authored -->

# Thread::round

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::round(std::int64_t decimals = 0) const -> Thread
```

## What It Does

Rounds every element to `decimals` decimal places (`0` means the nearest whole
number). Requires the thread to be `Series`-kind. For `decimals != 0`, the
implementation scales up by `10^decimals`, rounds to the nearest whole number, then
scales back down — the same multiply/round/divide approach most numeric libraries
use for decimal rounding.

## Parameters

**decimals** (`std::int64_t`, default `0`)
    Number of decimal places to round to. `0` rounds to the nearest integer value
    (still returned as a floating-point `Thread`).

## Returns

`Thread`
    A new `Series`-kind thread, same length as `*this`, holding the rounded values.

## Examples

Example 1. Round to the nearest whole number (the default).

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({2.4, 2.6, -2.4, -2.6}));
auto out = values.round();
std::cout << out;
```

### Output

```text
0  2
1  3
2  -2
3  -3

ptype: Series
```

Example 2. Round to a specific number of decimal places.

```cpp
auto pi_ish = Thread(pluss::make_series({3.14159, 2.71828, -1.23456}));
auto out2 = pi_ish.round(2);
std::cout << out2;
```

### Output

```text
0  3.14
1  2.72
2  -1.23

ptype: Series
```

Example 3. A non-`Series` thread throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b"}));
try {
  auto bad = notes.round();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::round is only valid for Series payload
```

## Notes

- Requires the calling thread to be `Series`-kind.
- `decimals != 0` is implemented as scale-up, round, scale-down — subject to the
  same floating-point representation limits as any multiply/divide-based rounding.
- A `NULL` cell on input stays `NULL` on output.

## See Also

- [`Thread::abs`](abs.md) / [`Thread::clip`](clip.md): other elementwise numeric
  transforms in the same family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ror.html">
    <span class="label">Previous</span>
    <span class="title">ror</span>
  </a>
  <a class="next-link" href="rpow.html">
    <span class="label">Next</span>
    <span class="title">rpow</span>
  </a>
</div>
