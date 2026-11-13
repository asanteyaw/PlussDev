<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab8ebd7e6187 -->

# Slate::bottom

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateView Slate::bottom(std::int64_t n, bool render = false) const
```

## What It Does

Returns the `n` **smallest**-valued entries, sorted ascending, as a `SlateView` —
the mirror image of [`top()`](top.md). `n` is clamped into `[0, size()]`, the same
convention as `top()`/[`head()`](head.md)/[`tail()`](tail.md) — a negative `n`
clamps to an empty result rather than throwing.

## Parameters

**n** (`std::int64_t`)
    Number of smallest-valued entries to keep. Clamped into `[0, size()]` — no
    error for a negative or oversized value.

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only.

## Returns

`SlateView`
    A view over the `n` smallest values, in ascending order, with `NULL`s sorted
    last.

## Examples

Example 1. The three smallest values, ascending.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.bottom(3);
std::cout << out;
```

### Output

```text
0  10
1  20
2  30

ptype: Series
```

Example 2. A negative `n` clamps to an empty result rather than throwing.

```cpp
auto out2 = scores.bottom(-1);
std::cout << out2;
```

### Output

```text
ptype: Series
```

## Notes

- `n` is clamped into `[0, size()]` — never throws for a negative or oversized
  value.
- `NULL` cells sort last, same as [`top()`](top.md).

## See Also

- [`Slate::top`](top.md): the largest-values counterpart.
- [`Slate::arrange`](arrange.md): the full sort this method takes a
  prefix of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="bounded.html">
    <span class="label">Next</span>
    <span class="title">bounded</span>
  </a>
</div>
