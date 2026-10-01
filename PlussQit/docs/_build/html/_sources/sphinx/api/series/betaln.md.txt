<!-- pluss-hand-authored -->

# Series::betaln

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::betaln(double other) const
Series Series::betaln(const Series& other) const
```

## A Stable Beta Normalizer In Log Space

`betaln()` evaluates the logarithm of the beta-function normalizer
elementwise:

```text
betaln(a, b) = lgamma(a) + lgamma(b) - lgamma(a + b)
```

The receiver supplies `a`. The argument supplies `b`, either as one scalar
constant or as a Series broadcast against the receiver. This is the preferred
form for log-likelihoods and normalizers: forming three gamma values and then
taking their quotient can overflow even when the logarithmic answer is finite.
Use [`beta()`](beta.md) only when the normalizer itself is needed.

## Overloads, Broadcasting, And Precision

| Call form | Right operand | Differentiable operands |
| --- | --- | --- |
| `a.betaln(value)` | One `double`, applied to every element | `a` |
| `a.betaln(b)` | A broadcast-compatible Series | `a` and `b` |

The Series overload follows ordinary trailing-axis broadcasting and returns
the broadcast shape. Both operands must be on the same platform. Integer and
boolean inputs promote to floating point; floating inputs retain their
supported promoted precision. No input is mutated.

For positive `a` and `b`, this is the logarithm of the Euler beta integral and
is symmetric in its arguments. The implementation is expressed through
`lgamma`, so its real-valued continuation and poles follow that primitive:
non-positive integer arguments can produce infinity or NaN. `betaln()` does
not clamp parameters into a probability-distribution domain.

## Reverse Mode

Writing `psi` for [`digamma()`](digamma.md):

```text
d/da betaln(a,b) = psi(a) - psi(a+b)
d/db betaln(a,b) = psi(b) - psi(a+b)
```

The Series overload records both paths, including reductions required by
broadcasting. The scalar overload treats `other` as a C++ constant.

## Examples

### 1. Broadcast One Scalar Parameter

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).betaln(3.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.0986123, -2.4849066, -3.4011974]
```

Because `B(1,b)=1/b`, the first result is `log(1/3)`.

### 2. Broadcast Two Series

```cpp
auto a = pluss::make_series(std::vector<double>{1.0, 2.0}, {2, 1});
auto b = pluss::make_series({2.0, 3.0, 4.0});
std::cout << a.betaln(b);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-0.69314718, -1.0986123, -1.3862944],
  [-1.7917595, -2.4849066, -2.9957323]]
```

Shapes `[2,1]` and `[3]` broadcast to `[2,3]`; this is not a pairwise outer
operation with special rules, but ordinary elementwise broadcasting.

### 3. Differentiate Both Series Operands

```cpp
auto a = pluss::make_series({2.0, 4.0});
auto b = pluss::make_series({3.0, 5.0});
a.tracks_slope_(true);
b.tracks_slope_(true);
a.betaln(b).sum().reverse();
std::cout << "da=" << a.sensitivity() << "\ndb=" << b.sensitivity();
```

### Output

```text
da=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1.0833333, -0.88452381]
db=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-0.58333333, -0.63452381]
```

Shape-incompatible operands and mixed platforms fail before computation.

## See Also

- [`Series::beta`](beta.md): exponentiate the log normalizer.
- [`Series::lgamma`](lgamma.md): the three-term numerical foundation.
- [`Series::digamma`](digamma.md): derivative of `lgamma`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="beta.html"><span class="label">Previous</span><span class="title">beta</span></a>
  <a class="next-link" href="bernoulli.html"><span class="label">Next</span><span class="title">bernoulli</span></a>
</div>
