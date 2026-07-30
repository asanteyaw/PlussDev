<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab3ce5b30a9d -->

# Thread::between

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::between(double a, double b, bool inclusive = false) const -> Thread
Thread::between(double a, double b, bool include_left, bool include_right) const -> Thread
Thread::between(std::string_view lo, std::string_view hi, bool inclusive = true) const -> Thread
Thread::between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const -> Thread
Thread::between(const Thread& lo, const Thread& hi, bool inclusive = true) const -> Thread
Thread::between(const Thread& lo, const Thread& hi, bool include_left, bool include_right) const -> Thread
```

## What It Does

A pure alias for [`btw()`](btw.md) — every `between()` overload forwards directly to the
identically-shaped `btw()` overload, with the exact same parameter meanings and the exact
same per-overload defaults for `inclusive`. Use whichever name reads better at the call
site; there is no behavioral difference.

```{important} Same default asymmetry as btw()
Like `btw()`, the numeric-scalar overload defaults to `inclusive = false` (open interval),
while the temporal/text-scalar and rowwise-`Thread` overloads default to `inclusive = true`
(closed interval). See [`btw()`](btw.md)'s important callout for the full explanation —
everything there applies here unchanged.
```

## Parameters

See [`Thread::btw`](btw.md) — `between()` takes the identical parameter set
(`a`/`b` or `lo`/`hi`, and `inclusive` or `include_left`/`include_right`) with identical
meanings.

## Returns

`Thread`
    A `Series`-kind thread of `Bool` values, one interval-membership result per row —
    identical to what the equivalent `btw()` call would return.

## Examples

Example 1. Numeric bounds with the default `inclusive` (open interval, same default as
`btw()`).

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.between(10.0, 30.0);
std::cout << out;
```

### Output

```text
0  false
1  true
2  false
ptype: Series
```

Example 2. Rowwise lower and upper bound threads, with shared inclusion.

```cpp
auto values = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto lower = Thread(pluss::make_series({10.0, 20.0, 35.0, 35.0, 45.0}));
auto upper = Thread(pluss::make_series({15.0, 25.0, 35.0, 45.0, 50.0}));

auto out = values.between(lower, upper, true);
std::cout << out;
```

### Output

```text
0  true
1  true
2  false
3  true
4  true
ptype: Series
```

Example 3. Temporal scalar bounds with the default `inclusive` (closed interval here,
the opposite default from the numeric overload — see the important callout above).

```cpp
auto dates = Thread(pluss::make_timeline({
    "2024-01-05T09:00:00Z",
    "2024-01-18T12:00:00Z",
    "2024-02-02T08:00:00Z",
}));

auto out = dates.between("2024-01-01T00:00:00Z", "2024-01-31T23:59:59Z");
std::cout << out;
```

### Output

```text
0  true
1  true
2  false
ptype: Series
```

## Notes

- `between()` is exactly `btw()` under a different name — every overload, every default,
  and every edge case documented on [`btw()`](btw.md) (including the swap-if-inverted
  behavior and the throw on a mismatched payload kind) applies here unchanged.
- Prefer whichever of `between()`/`btw()` matches the vocabulary of the surrounding code;
  there's no performance or behavioral reason to pick one over the other.

## See Also

- [`Thread::btw`](btw.md): the full documentation this page mirrors — including the
  numeric-vs-temporal default asymmetry, the bound-swap behavior, and the throw case.
- [`Thread::compare`](compare.md): the general comparison surface both `between()` and
  `btw()` are built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
