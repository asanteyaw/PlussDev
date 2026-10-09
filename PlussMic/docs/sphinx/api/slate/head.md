<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cc1a9b3bb692 -->

# Slate::head

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::head(std::int64_t n = 5, bool render = false) const -> SlateView
```

## What It Does

Returns the first `n` entries of the slate as a `SlateView` — a `Slate` subclass
(it inherits every `Slate` method and renders identically via `operator<<`) that
additionally carries a display title (`"head"`) and an opt-in auto-render behavior
for interactive/terminal contexts. `n` is silently clamped into `[0, size()]` —
`head()` never throws for an out-of-range `n`, the same convention
[`top()`](top.md)/[`bottom()`](bottom.md) use.

```{note} SlateView is a Slate
Every method documented elsewhere in this reference — arithmetic, string ops,
reductions, everything — works directly on the `SlateView` returned here, since it
publicly inherits from `Slate`. The `SlateView` wrapper only adds a couple of
`show()` overloads and a distinct stream-insertion operator (which renders exactly
like `Slate`'s own).
```

## Parameters

**n** (`std::int64_t`, default `5`)
    Number of leading entries to keep. Values below `0` clamp to `0`; values above
    `size()` clamp to `size()` — no error either way.

**render** (`bool`, default `false`)
    When `true`, the returned view may auto-render itself if it reaches the end of
    its scope without being explicitly printed (a convenience for interactive/REPL
    contexts). Has no effect on what an explicit `std::cout << ...` prints, which is
    what every example on this page uses.

## Returns

`SlateView`
    A view over the first `n` entries (or every entry, if `n >= size()`).

## Examples

Example 1. The default `n = 5` on a shorter slate returns everything.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto out = scores.head(2);
std::cout << out;
```

### Output

```text
0  10
1  20

ptype: Series
```

Example 2. `n` larger than `size()` clamps — no error, just the whole slate back.

```cpp
auto out2 = scores.head(100);
std::cout << out2;
```

### Output

```text
0  10
1  20
2  30

ptype: Series
```

## Notes

- `n` is clamped into `[0, size()]`, never throws.
- The returned `SlateView` is a full `Slate` — chain any other method directly on
  it.
- `render` only affects auto-render-on-scope-exit behavior in interactive contexts,
  not what an explicit stream insertion prints.

## See Also

- [`Slate::tail`](tail.md): the trailing-entries counterpart.
- [`Slate::view`](view.md): an arbitrary `[start, stop)` window, not anchored to
  either end.
- [`Slate::top`](top.md) / [`Slate::bottom`](bottom.md): value-sorted previews,
  rather than positional ones.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
