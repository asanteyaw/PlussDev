<!-- pluss-hand-authored -->

# Series::besselk

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::besselk() const
Series Series::besselk(double order) const
```

## The Decaying Modified Bessel Family

`besselk()` evaluates the modified Bessel function of the second kind,
`K_order(x)`, elementwise. It solves the same modified Bessel equation as
[`besseli()`](besseli.md), but chooses the independent solution that decays
exponentially for positive `x`. This is the branch used for radial effects that
must vanish with distance rather than grow.

The no-argument form means order zero. The overload accepts one finite real
order shared by every lane. `K_v` is even in its order: `K_-v(x) = K_v(x)`.
Its real-valued domain is the positive axis. At zero it has a positive
singularity and returns `inf`; negative arguments return `NaN` lane by lane.
Those values describe the mathematical boundary. A non-finite order is instead
a malformed call and raises an exception.

The operation allocates a shape-preserving floating result. Integral input is
promoted to `float64`; the receiver is not modified. The positive-real kernel
uses a stable integral representation, avoiding the nearly singular
`sin(pi*order)` quotient that loses precision close to integer orders.

## Reverse Mode

The derivative is negative one half of the adjacent-order sum:

```text
d K_v(x) / dx = -(K_(v-1)(x) + K_(v+1)(x)) / 2
```

For order zero, this becomes `-K_1(x)`, matching the fact that `K_0` decreases
on the positive axis.

## Examples

Example 1. The default order decays rapidly as the argument grows.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besselk());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.9244, 0.4210, 0.1139, 0.0112]
```

Example 2. Order one is a distinct member of the same decaying family.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0.5, 1.0, 2.0, 4.0}).besselk(1.0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.6564, 0.6019, 0.1399, 0.0125]
```

Example 3. Domain outcomes are carried independently for each lane.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({-1.0, 0.0, 1.0}).besselk());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[nan, inf, 0.4210]
```

Example 4. Reverse mode for `K_0` produces the negative `K_1` values.

```cpp
auto x = pluss::make_series({0.5, 1.0, 2.0});
x.tracks_slope_(true);
x.besselk().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1.6564, -0.6019, -0.1399]
```

Example 5. Order sign does not change the result.

```cpp
const auto x = pluss::make_series({0.5, 1.0, 2.0});
std::cout << std::boolalpha
          << x.besselk(-1.5).allclose(x.besselk(1.5)) << "\n";
```

### Output

```text
true
```

## See Also

- [`besseli()`](besseli.md) is finite at zero and grows on the positive axis.
- [`bessely()`](bessely.md) is the singular oscillatory second solution.
