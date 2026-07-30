<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 65c39a7e6106 -->

# Thread::view

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::view(std::int64_t start, std::int64_t stop, bool render = false) const -> ThreadView
```

## What It Does

Returns an arbitrary `[start, stop)` positional window as a `ThreadView` — the
general form [`head()`](head.md)/[`tail()`](tail.md) are anchored special cases of.
Delegates directly to [`slice()`](slice.md) internally, so it shares that method's
Python-style negative-index and clamping behavior: `start`/`stop` support negative
indices (counted from the end), both are clamped into `[0, size()]`, and a `stop`
that ends up before `start` produces an empty (not erroring) result.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting position. Negative values count from the end.

**stop** (`std::int64_t`)
    Exclusive ending position. Negative values count from the end.

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only.

## Returns

`ThreadView`
    A view over positions `[start, stop)`, clamped into range.

## Examples

Example 1. A plain forward window.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = scores.view(1, 3);
std::cout << out;
```

### Output

```text
0  20
1  30

ptype: Series
```

Example 2. Negative indices count from the end, same as Python slicing.

```cpp
auto out2 = scores.view(-3, -1);
std::cout << out2;
```

### Output

```text
0  30
1  40

ptype: Series
```

Example 3. Out-of-range bounds clamp; a `stop` before `start` yields an empty
result rather than an error.

```cpp
auto out3 = scores.view(3, 1);
std::cout << out3;
```

### Output

```text
ptype: Series
```

## Notes

- Delegates to [`slice()`](slice.md) — shares its negative-index and clamping
  semantics exactly.
- Never throws for out-of-range or inverted bounds; an inverted `[start, stop)`
  just produces an empty view.

## See Also

- [`Thread::head`](head.md) / [`Thread::tail`](tail.md): anchored special cases of
  this method.
- [`Thread::slice`](slice.md): the plain-`Thread`-returning method this delegates
  to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="weigh.html">
    <span class="label">Next</span>
    <span class="title">weigh</span>
  </a>
</div>
