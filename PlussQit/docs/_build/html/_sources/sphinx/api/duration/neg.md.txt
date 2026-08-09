<!-- pluss-hand-authored -->

# Duration::neg

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::operator-() const;
```

## What It Does

Unary `-Duration` negates every element, flipping "forward in time" to
"backward in time" and back. It is equivalent to `pluss::duration::nanos(0) -
self`, but implemented directly rather than via a subtraction.

## Parameters

None — this is a unary operator on `self`.

## Returns

`Duration`
    A new duration with every element's sign flipped. Magnitude is unchanged.

## Examples

Example 1. Negate a positive duration.

```cpp
auto one_hour = pluss::duration::hours(1);
std::cout << pluss::duration::print(-one_hour);
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
-0 days 01:00:00.000000000
```

The leading `-` sits in front of the whole `"0 days HH:MM:SS..."` rendering,
not folded into the day count — a negative one-hour duration prints as
`-0 days 01:00:00.000000000`, not `-1 days 23:00:00.000000000` or similar.

## Notes

- Negating twice returns to the original value: `-(-d) == d`.
- A negative `Duration` is a perfectly valid value, not an error state — see
  [`abs`](abs.md)/[`sign`](sign.md) for recovering magnitude and direction
  separately.

## See Also

- [`sub`](sub.md): binary subtraction, related by `a - b == a + (-b)`.
- [`abs`](abs.md): magnitude without direction, the complementary transform.
- [`sign`](sign.md): direction alone (`-1`, `0`, or `1`).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
