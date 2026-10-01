<!-- pluss-hand-authored -->
# pluss::duration::random
[Back to Duration Factory API Reference](../../duration_factory_api_reference)

## Signature
```cpp
Duration pluss::duration::random(integer_range sizes,
                                 const Duration& lo,
                                 const Duration& hi,
                                 std::uint64_t seed = 0);
```

`random()` samples integer nanoseconds uniformly from the half-open interval
`[lo, hi)`. Bounds must be finite scalar Durations with `hi > lo`. A nonzero
seed is reproducible; zero requests the library's entropy-seed sequence and
must not be documented with fixed values. Sampling remains integer-exact even
for ranges wider than floating-point precision.

## Examples
```cpp
auto sample = pluss::duration::random(
    {2,3}, pluss::duration::nanos(10), pluss::duration::nanos(20), 42);
for (auto x : sample.host()) std::cout << x << ' ';
```
```text
16 18 15 10 10 16
```

```cpp
auto a = pluss::duration::random({4}, pluss::duration::seconds(-1),
    pluss::duration::seconds(1), 91);
auto b = pluss::duration::random({4}, pluss::duration::seconds(-1),
    pluss::duration::seconds(1), 91);
std::cout << std::boolalpha << (a.host() == b.host());
```
```text
true
```

```cpp
pluss::duration::random({2}, pluss::duration::seconds(2),
    pluss::duration::seconds(1), 42);
// throws: pluss::duration::random: expected hi > lo
```

