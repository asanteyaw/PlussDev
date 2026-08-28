<!-- pluss-hand-authored -->

# Series::log

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::log() const -> Series
```

## What It Does

Applies the natural logarithm elementwise.

`log()` is the inverse-style partner to [`Series::exp`](exp.md) on positive
inputs. It preserves shape and transforms each element independently.

The important point is domain sensitivity:

- positive inputs produce finite real outputs
- `1` maps to `0`
- values between `0` and `1` map to negative outputs
- `0` maps to `-inf`
- negative inputs map to `nan`

So `log()` is not mainly about shape or broadcasting. It is about input domain.

## Parameters

None.

## Returns

`Series`
    A new `Series` with the same shape as `self`, where each element has been
    replaced by its natural logarithm.

## Gradient

For positive inputs, `log()` has the familiar reciprocal derivative:

- the forward value is `log(x)`
- the backward multiplier is `1 / x`

That means gradients become larger near zero and smaller for larger positive
values.

## Examples

Example 1. Inputs chosen to produce exact whole-number logs.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.718281828459045, 7.38905609893065}).log());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 1.0000, 2.0000]
```

Example 2. Fractions between `0` and `1` produce negative outputs.

```cpp
std::cout << pluss::make_series({0.25, 0.5, 1.0, 4.0}).log();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1.3862944, -0.69314718, 0, 1.3862944]
```

Example 3. The reverse-mode sensitivity is the reciprocal of the positive input.

```cpp
auto x = pluss::make_series({1.0, 2.0, 4.0});
x.tracks_slope_(true);

auto out = x.log();
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dx=" << x.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0.69314718, 1.3862944]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0.5, 0.25]
```

Example 4. Domain edges do not throw here; they surface as special floating
values.

```cpp
std::cout << pluss::make_series({1.0, 0.0, -1.0}).log();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, -inf, nan]
```

## Gotchas And Failure Modes

- `log()` is only finite on positive inputs.
- `0` yields `-inf`, which is often more dangerous in downstream arithmetic
  than an immediate exception because it can travel further before you notice.
- Negative inputs yield `nan`.
- Gradients become large near zero on the positive side.

## Summary

Teach `log()` as a domain-sensitive unary transform. The key lesson is not
shape, but admissible values: positive inputs are ordinary, zero becomes
`-inf`, and negatives become `nan`.

## See Also

- [`Series::exp`](exp.md): inverse-style counterpart on valid domains.
- [`Series::sqrt`](sqrt.md): another domain-sensitive unary transform.
- [`Series::sign`](sign.md): cheap sign inspection before deciding whether a
  `log()` call is sensible.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
