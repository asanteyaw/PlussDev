<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14cfaba3a439 -->

# Slate::to_string

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
std::string Slate::to_string(const display::DisplayOptions& options = {}) const
```

## What It Does

Renders the slate's actual row values — index, values, and the trailing `ptype:` footer —
as a plain `std::string`, using the same rendering [`show()`](show.md) and `operator<<`
use, just returned instead of written to a stream. This is the render-to-string counterpart
of [`info_string()`](info_string.md), which returns *metadata* instead of row values.

```{important} No trailing newline — unlike show()/operator<<
The string `to_string()` returns ends right after the `ptype:` line, with **no** trailing
`\n`. `show()` and `operator<<`, by contrast, always end their stream output with a
trailing newline. If you're switching from `std::cout << slate` to
`std::cout << slate.to_string()`, add your own `"\n"` if you want the same spacing. See
Example 1.
```

## Parameters

**options** (`const display::DisplayOptions&`, optional)
    Rendering options — same type `show()` accepts. For `Slate`, the fields that actually
    matter are `terminal_width`/`terminal_height` (`<= 0` auto-detects from the real
    terminal; a smaller height lowers how many rows are shown before truncating to a `...`
    ellipsis row) and `mode` (`Auto`/`Compact`/`Wide`, which adjusts cell-width presets).

```{important} show_case_index and show_shape_footer have no effect on Slate
Those two flags exist on `DisplayOptions` and are honored by `DataFrame` rendering, but
`Slate`'s renderer never consults them — the row-index column and the `ptype:` footer are
always present when rendering a `Slate`, regardless of how you set these two fields.
```

## Returns

`std::string`
    The rendered slate — row index, values, and (by default) a `ptype:` footer — with no
    trailing newline.

## Examples

Example 1. Default rendering — note there's no trailing newline, so a second `std::cout`
call continues on the same line unless you add one yourself.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0}));

auto out = scores.to_string();
std::cout << out << "|END";
```

### Output

```text
0  10
1  20
2  30
ptype: Series|END
```

Example 2. A long slate truncates to a head/tail preview with a `...` ellipsis row —
how many rows are shown before truncating is driven by `terminal_height` (here forced small
to make the truncation deterministic regardless of the real terminal).

```cpp
display::DisplayOptions options;
options.terminal_height = 12;

std::vector<double> values;
for (int i = 0; i < 30; ++i) {
  values.push_back(static_cast<double>(i));
}
auto series = Slate(pluss::make_series(values));

auto out = series.to_string(options);
std::cout << out;
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

- Renders row values, not metadata — use [`info_string()`](info_string.md) instead for a
  name/kind/dtype/null-count summary.
- No trailing newline in the returned string — see the important callout above.
- `to_string()`, [`show()`](show.md), and `operator<<` all share the same underlying
  renderer and accept the same `DisplayOptions`; the only difference is destination
  (string vs. stream) and the trailing-newline detail above.

## See Also

- [`Slate::show`](show.md): render straight to a stream instead of returning a string —
  ends with a trailing newline, unlike this method.
- [`Slate::info_string`](info_string.md): the metadata-summary counterpart — name, kind,
  dtype, and null counts, not row values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="top.html">
    <span class="label">Next</span>
    <span class="title">top</span>
  </a>
</div>
