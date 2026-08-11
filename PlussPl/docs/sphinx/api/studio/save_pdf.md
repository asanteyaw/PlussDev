<!-- pluss-hand-authored -->

# Studio::save_pdf

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
void Studio::save_pdf(std::string_view path, PdfEngine engine = PdfEngine::Auto) const
```

## What It Does

Renders the figure to PDF and writes it to `path` on disk. Which rendering path is used
depends on `engine`:

- `PdfEngine::Auto` (the default) and `PdfEngine::Internal` both take the **native,
  dependency-free PDF emitter** — the same one [`to_pdf()`](to_pdf.md) always uses. As of
  this API surface, `Auto` and `Internal` behave identically; `Auto` exists as the forward
  -compatible default in case engine auto-selection logic is added later.
- `PdfEngine::SvgConvert` instead renders to a temporary SVG file, invokes an **external**
  SVG→PDF converter on it (`rsvg-convert`, `inkscape`, or ImageMagick's `magick`/`convert`,
  whichever is found first), copies the result to `path`, and deletes the temporary SVG.

```{important} SvgConvert requires an external tool; the default engine does not
`PdfEngine::Auto`/`Internal` always succeed with zero external dependencies. Only
`PdfEngine::SvgConvert` can fail for environmental reasons — if none of `rsvg-convert`,
`inkscape`, or `magick`/`convert` are found on the system, it throws
`std::runtime_error("save_pdf(): SvgConvert engine requested, but no SVG->PDF converter
was available (rsvg-convert, inkscape, magick/convert).")`. Reach for `SvgConvert` only
when there's a specific reason to prefer an external tool's rendering over the native
emitter (e.g. matching another tool's exact PDF output characteristics) — the default is
reliable everywhere.
```

```{important} path must end in .pdf, case-insensitively, or this throws immediately
Exactly like [`save_svg()`](save_svg.md), the extension check runs before any rendering,
for every `engine` value.
```

## Parameters

**path** (`std::string_view`)
    The destination file path. Must end in `.pdf` (case-insensitive).

**engine** (`PdfEngine`, default `PdfEngine::Auto`)
    Which rendering path to use: `Auto`, `Internal` (both native), or `SvgConvert`
    (external tool).

## Returns

`void`

## Examples

Example 1. The default engine (`Auto`) — the native emitter, no external tools required.

```cpp
using namespace pluss::pl;

auto fig = studio("save_pdf_auto");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

fig.save_pdf("demo_auto.pdf");
```

![Output of Example 1](img/to_svg-1-basic.svg)

Example 2. `PdfEngine::Internal`, explicitly — currently identical output to `Auto`.

```cpp
using namespace pluss::pl;

auto fig = studio("save_pdf_internal");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

fig.save_pdf("demo_internal.pdf", PdfEngine::Internal);
```

Example 3. A path with the wrong extension throws before any rendering, regardless of
`engine`.

```cpp
using namespace pluss::pl;

auto fig = studio("save_pdf_wrong_ext");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

try {
  fig.save_pdf("demo.svg");
} catch (const std::runtime_error& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
save_pdf(): filename extension '.svg' does not match expected '.pdf'
```

## Notes

- Not `noexcept` — throws on an extension mismatch (any engine) or, for
  `PdfEngine::SvgConvert` specifically, when no external converter is found.
- `Auto` and `Internal` are currently equivalent; prefer `Auto` unless there's a reason to
  pin to the native engine explicitly regardless of future `Auto` changes.
- `SvgConvert`'s temporary SVG file is written to (and cleaned up from) a scratch temp
  directory, not `path`'s own directory.
- Extension check is case-insensitive, same as `save_svg()`.

## See Also

- [`Studio::to_pdf`](to_pdf.md): the in-memory equivalent, always using the native engine.
- [`Studio::save_svg`](save_svg.md) / [`save_png`](save_png.md): the SVG and PNG
  equivalents.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_svg.html">
    <span class="label">Previous</span>
    <span class="title">save_svg</span>
  </a>
  <a class="next-link" href="save_png.html">
    <span class="label">Next</span>
    <span class="title">save_png</span>
  </a>
</div>
