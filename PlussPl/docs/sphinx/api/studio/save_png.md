<!-- pluss-hand-authored -->

# Studio::save_png

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
void Studio::save_png(std::string_view path, std::int64_t dpi = 144) const
```

## What It Does

Renders the figure to a raster PNG image and writes it to `path`. Unlike
[`save_svg()`](save_svg.md)/[`save_pdf()`](save_pdf.md), there is no native PNG emitter —
`save_png()` **always** renders to a temporary SVG first, then rasterizes it with an
**external** tool (`rsvg-convert`, `inkscape`, or ImageMagick's `magick`/`convert`,
whichever is found first), at a pixel size derived from the figure's
[`extent()`](extent.md) and the requested `dpi` (`pixels = points * dpi / 96`). The
temporary SVG is deleted afterward either way.

```{important} save_png() always needs an external rasterizer -- there is no native fallback
Unlike `save_pdf()` (whose default `Auto`/`Internal` engines need nothing external),
`save_png()` has no dependency-free path at all. If none of `rsvg-convert`, `inkscape`, or
`magick`/`convert` are installed, it throws
`std::runtime_error("save_png(): unable to rasterize SVG. Install one of: rsvg-convert,
inkscape, or ImageMagick (magick).")`. If PNG export needs to work in an environment with
no guarantee of these tools being present, `save_svg()` (always available) is the safer
fallback to build around.
```

## Parameters

**path** (`std::string_view`)
    The destination file path. Must end in `.png` (case-insensitive).

**dpi** (`std::int64_t`, default `144`)
    The rasterization density. Must be positive. Higher values produce a larger pixel
    image (and file) at the same physical figure size — see Example 2.

## Returns

`void`

## Examples

Example 1. The default `dpi` of `144`.

```cpp
using namespace pluss::pl;

auto fig = studio("save_png_default");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

fig.save_png("demo.png");
```

![Output of Example 1](img/to_svg-1-basic.svg)

Example 2. A higher `dpi` (`300`) produces a larger raster file than the `144`-dpi default
for the same figure.

```cpp
using namespace pluss::pl;

auto fig = studio("save_png_hi_dpi");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

fig.save_png("demo_144.png", 144);
fig.save_png("demo_300.png", 300);
```

Example 3. `dpi <= 0` throws immediately, before any rendering.

```cpp
using namespace pluss::pl;

auto fig = studio("save_png_bad_dpi");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

try {
  fig.save_png("demo.png", 0);
} catch (const std::invalid_argument& e) {
  std::cout << e.what() << "\n";
}

try {
  fig.save_png("demo.png", -50);
} catch (const std::invalid_argument& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
save_png(): dpi must be positive
save_png(): dpi must be positive
```

Example 4. A path with the wrong extension throws before any rendering.

```cpp
using namespace pluss::pl;

auto fig = studio("save_png_wrong_ext");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

try {
  fig.save_png("demo.svg");
} catch (const std::runtime_error& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
save_png(): filename extension '.svg' does not match expected '.png'
```

## Notes

- Not `noexcept` — throws on an extension mismatch, a non-positive `dpi`, or (unlike
  `save_svg()`/`save_pdf()`'s default engine) whenever no rasterizer tool is installed.
- Always depends on an external tool; there is no native fallback. See the important
  callout above.
- Output pixel dimensions are computed from the figure's `extent()` (in points) and `dpi`:
  `pixels = points * dpi / 96`, with a `64`px floor on each dimension.
- Extension check is case-insensitive, same as `save_svg()`/`save_pdf()`.

## See Also

- [`Studio::save_svg`](save_svg.md): the always-available, dependency-free alternative when
  a raster format isn't strictly required.
- [`Studio::save_pdf`](save_pdf.md): the PDF equivalent, whose default engine needs no
  external tool (only its optional `SvgConvert` engine does).
- [`Studio::extent`](extent.md): the figure size (in points) that `dpi` scales against.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_pdf.html">
    <span class="label">Previous</span>
    <span class="title">save_pdf</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
