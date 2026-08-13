<!-- pluss-hand-authored -->

# Studio::to_pdf

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
[[nodiscard]] std::string Studio::to_pdf() const
```

## What It Does

The PDF counterpart of [`to_svg()`](to_svg.md): renders the figure's current state to a
complete PDF document and returns its raw bytes as a `std::string` (used here purely as a
byte buffer — the content is binary, not text). [`save_pdf()`](save_pdf.md) is `to_pdf()`
plus writing the result to a file.

Unlike `save_pdf()`, `to_pdf()` has no `PdfEngine` parameter — it always uses the native,
dependency-free PDF emitter (the same one `save_pdf()` uses for `PdfEngine::Auto` and
`PdfEngine::Internal`). There is no `to_pdf()` equivalent of `save_pdf()`'s
`PdfEngine::SvgConvert` path, since that path is specifically about converting an
intermediate file on disk with an external tool — `to_pdf()` never touches disk at all.

```{note} PDF output isn't necessarily larger than SVG
It's tempting to assume a "richer" binary format like PDF is always the bigger payload, but
for simple figures the reverse can hold — a minimal line plot's native PDF was
byte-for-byte *smaller* than its SVG in testing (SVG's verbose, human-readable XML markup
outweighs PDF's more compact binary encoding at this scale). Don't assume a size
relationship in either direction; measure it for the figure in question if it matters.
```

## Parameters

*(none)*

## Returns

`std::string`
    The complete PDF document as raw bytes, beginning with the standard `%PDF-` header.

## Examples

Example 1. A basic figure rendered to a PDF byte string.

```cpp
using namespace pluss::pl;

auto fig = studio("to_pdf_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

std::string pdf = fig.to_pdf();
std::cout << (pdf.rfind("%PDF-", 0) == 0) << "\n";
std::cout << (!pdf.empty()) << "\n";
```

### Output

```text
1
1
```

## Notes

- `[[nodiscard]]`, `const`.
- Re-renders from scratch on every call, same as `to_svg()` — no caching.
- Returned as `std::string` purely as a byte container; the content is binary PDF data, not
  text. Don't print it directly to a text stream expecting readable output — check its
  magic-number prefix (`%PDF-`) or write it straight to a file instead.
- Always uses the native PDF emitter — no external tool dependency, unlike `save_pdf()`'s
  optional `PdfEngine::SvgConvert` path.

## See Also

- [`Studio::save_pdf`](save_pdf.md): `to_pdf()`'s native-engine path, plus writing to a
  file, plus the `PdfEngine::SvgConvert` alternative this method doesn't offer.
- [`Studio::to_svg`](to_svg.md): the SVG equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_svg.html">
    <span class="label">Previous</span>
    <span class="title">to_svg</span>
  </a>
  <a class="next-link" href="save_svg.html">
    <span class="label">Next</span>
    <span class="title">save_svg</span>
  </a>
</div>
