<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14686413b97c -->

# Slate::top

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::top(std::int64_t n, bool render = false) const -> SlateView
```

## What It Does

Returns the `n` **largest**-valued entries, sorted descending, as a `SlateView`
(see [`head()`](head.md) for the shared `SlateView` mechanics). `n` is clamped
into `[0, size()]`, the same as [`head()`](head.md)/[`tail()`](tail.md) — a
negative `n` clamps to `0` (an empty result) rather than throwing, and an `n`
larger than `size()` clamps to `size()`.

## Parameters

**n** (`std::int64_t`)
    Number of largest-valued entries to keep. Clamped into `[0, size()]` — no
    error for a negative or oversized value.

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only.

## Returns

`SlateView`
    A view over the `n` largest values, in descending order, with `NULL`s sorted
    last.

## Examples

Example 1. The three largest values, descending.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({30.0, 10.0, 50.0, 20.0, 40.0}));
auto out = scores.top(3);
std::cout << out;
```

### Output

```text
0  50
1  40
2  30

ptype: Series
```

Example 2. A negative `n` clamps to an empty result rather than throwing.

```cpp
auto out2 = scores.top(-1);
std::cout << out2;
```

### Output

```text
ptype: Series
```

## Notes

- `n` is clamped into `[0, size()]` — never throws for a negative or oversized
  value, matching [`head()`](head.md)/[`tail()`](tail.md).
- `NULL` cells sort last, so they only appear in the result if `n` is large enough
  to reach past every valid value.

## See Also

- [`Slate::bottom`](bottom.md): the smallest-values counterpart.
- [`Slate::head`](head.md) / [`Slate::tail`](tail.md): positional (not
  value-sorted) previews, with the same clamping convention.
- [`Slate::arrange`](arrange.md): the full sort this method takes a
  prefix of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
