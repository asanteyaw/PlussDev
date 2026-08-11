<!-- pluss-hand-authored -->

# Studio::save_svg

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
void Studio::save_svg(std::string_view path) const
```

## What It Does

Renders the figure exactly as [`to_svg()`](to_svg.md) would, and writes the result
directly to `path` on disk. This is the primary way to persist a figure as a standalone
SVG file.

```{important} path must end in .svg, case-insensitively, or this throws immediately
Before any rendering or file I/O happens, the extension of `path` is checked against
`.svg` (case-insensitively — `.SVG`, `.Svg`, etc. all pass). A mismatch throws a
`std::runtime_error` (specifically `pluss::Error`, an internal type derived from it) with a
message naming both the actual and expected extension, and nothing is written to disk. See
Example 2.
```

## Parameters

**path** (`std::string_view`)
    The destination file path. Must end in `.svg` (case-insensitive). Parent directories
    are not created automatically — the path's containing directory must already exist.

## Returns

`void`

## Examples

Example 1. Saving a figure to an actual `.svg` file on disk.

```cpp
using namespace pluss::pl;

auto fig = studio("save_svg_check");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

fig.save_svg("demo.svg");
```

![Output of Example 1](img/to_svg-1-basic.svg)

Example 2. A path with the wrong extension throws before writing anything.

```cpp
using namespace pluss::pl;

auto fig = studio("save_svg_wrong_ext");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

try {
  fig.save_svg("demo.png");
} catch (const std::runtime_error& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
save_svg(): filename extension '.png' does not match expected '.svg'
```

## Notes

- Not `noexcept` — throws on an extension mismatch, and file I/O can fail for other
  reasons (permissions, missing parent directory) as an unhandled `std::ios`/filesystem
  error.
- Extension check is case-insensitive.
- Does not create missing parent directories — ensure the destination directory exists
  first.
- Equivalent to `to_svg()` plus a file write; see [`to_svg()`](to_svg.md) if a string in
  memory is wanted instead of a file on disk.

## See Also

- [`Studio::to_svg`](to_svg.md): the in-memory equivalent this method writes to disk.
- [`Studio::save_pdf`](save_pdf.md) / [`save_png`](save_png.md): the PDF and PNG
  equivalents.
- [`Studio::show`](show.md): opens the figure in a viewer instead of persisting it to disk.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_pdf.html">
    <span class="label">Previous</span>
    <span class="title">to_pdf</span>
  </a>
  <a class="next-link" href="save_pdf.html">
    <span class="label">Next</span>
    <span class="title">save_pdf</span>
  </a>
</div>
