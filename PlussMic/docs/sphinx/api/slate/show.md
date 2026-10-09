<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c92c98fe3f4d -->

# Slate::show

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::show(const display::DisplayOptions& options = {}) const -> void
Slate::show(std::ostream& os, const display::DisplayOptions& options = {}) const -> void
```

## What It Does

Renders the slate's row values — the same rendering `operator<<` and
[`to_string()`](to_string.md) use — as a side effect, rather than returning it. The two
overloads behave differently in one important way, not just "default stream vs. explicit
stream":

- **`show(os, options)`** always prints plain text straight to `os`, exactly like
  `os << slate`.
- **`show(options)`** (no stream argument) first checks whether a GUI backend is available
  and configured. If one is, it opens an interactive viewer window instead of printing to
  `std::cout`; only if no GUI backend is available does it fall back to printing to
  `std::cout`.

```{important} show() with no stream argument may open a GUI window instead of printing
On a machine with a GUI session available (a typical desktop dev environment), calling
`slate.show()` with no arguments and default options can resolve to a native or web-based
viewer window rather than writing to `std::cout` — and can block waiting for that window.
If you want guaranteed plain-text output to the console, either pass `std::cout` explicitly
(`slate.show(std::cout)`) or use [`to_string()`](to_string.md)/`operator<<`, neither of
which ever opens a GUI window.
```

## Parameters

**options** (`const display::DisplayOptions&`, optional)
    Rendering options — see [`to_string()`](to_string.md) for which fields actually affect
    `Slate` output, plus `backend` (`Auto`/`Terminal`/`NativeWebView`/`PlussWebView`),
    which only matters for the no-stream overload's GUI-vs-terminal decision.

**os** (`std::ostream&`, `show(os, options)` overload only)
    The destination stream. Always renders plain text here — this overload never opens a
    GUI window.

## Returns

`void`
    Renders as a side effect; produces no return value.

## Examples

Example 1. `show(os)` with an explicit stream always prints plain text — safe to use
anywhere, including scripts and tests.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));

scores.show(std::cout);
```

### Output

```text
0  10
1  20
2  30
ptype: Series
```

Example 2. Passing `DisplayOptions` alongside an explicit stream still renders plain text —
only the *no-stream* overload considers opening a GUI window.

```cpp
display::DisplayOptions options;
options.terminal_height = 12;

std::vector<double> values;
for (int i = 0; i < 30; ++i) {
  values.push_back(static_cast<double>(i));
}
auto series = Slate(pluss::make_series(values));

series.show(std::cout, options);
```

### Output

```text
  0  0
  1  1
  2  2
  3  3
  4  4
...  ...
 27  27
 28  28
 29  29
ptype: Series
```

## Notes

- `show(os, options)` is equivalent to `os << slate` (plus whichever `options` you pass) —
  always plain text, never a GUI window.
- `show(options)` (no stream) may open a GUI viewer window instead of printing, depending on
  the runtime environment — see the important callout above. Pass `std::cout` explicitly if
  you always want plain text.
- Shares its rendering (and the same "row-index column and `ptype:` footer are always
  present for `Slate`" behavior) with [`to_string()`](to_string.md) — see that page for
  which `DisplayOptions` fields matter.

## See Also

- [`Slate::to_string`](to_string.md): render to a `std::string` instead of a stream —
  never opens a GUI window, and documents which `DisplayOptions` fields actually affect
  `Slate` rendering.
- [`Slate::info`](info.md) / [`Slate::info_string`](info_string.md): render metadata
  (name, kind, dtype, null counts) instead of row values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="shuffle.html">
    <span class="label">Next</span>
    <span class="title">shuffle</span>
  </a>
</div>
