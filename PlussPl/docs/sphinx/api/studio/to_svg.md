<!-- pluss-hand-authored -->

# Studio::to_svg

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
[[nodiscard]] std::string Studio::to_svg() const
```

## What It Does

Renders the figure's current state to a complete, self-contained SVG document and returns
it as a `std::string`. This is the lowest-level rendering entry point in `Studio` —
[`save_svg()`](save_svg.md) is `to_svg()` plus writing the result to a file, and even
[`show()`](show.md) calls `to_svg()` internally before wrapping the payload in an HTML
viewer page. Every method throughout this documentation set that needs to inspect or embed
a figure's rendered output (verifying an example's rendered result, for instance) goes
through `to_svg()`.

Rendering is stateless with respect to the call itself — every call re-renders from
whatever the figure's current state is at that moment. There's no caching or "dirty" flag:
mutate the figure, call `to_svg()` again, and the new string reflects the new state. See
Example 2.

## Parameters

*(none)*

## Returns

`std::string`
    A complete SVG document, starting with an XML/SVG declaration and ending with
    `</svg>`.

## Examples

Example 1. A basic figure rendered to an SVG string.

```cpp
using namespace pluss::pl;

auto fig = studio("to_svg_check");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

std::string svg = fig.to_svg();
std::cout << (svg.rfind("<?xml", 0) == 0 || svg.rfind("<svg", 0) == 0) << "\n";
std::cout << (svg.find("</svg>") != std::string::npos) << "\n";
```

### Output

```text
1
1
```

![Output of Example 1](img/to_svg-1-basic.svg)

Example 2. Two calls to `to_svg()` around a mutation — the second call reflects the new
state, since there's no caching.

```cpp
using namespace pluss::pl;

auto fig = studio("to_svg_live");
auto& pane = fig.pane2();
flash::lineplot(pane, std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

std::string svg_before = fig.to_svg();
fig.headline("Now With A Headline");
std::string svg_after = fig.to_svg();

std::cout << (svg_before != svg_after) << "\n";
```

### Output

```text
1
```

## Notes

- `[[nodiscard]]` — discarding the return value is a compiler warning, and pointless since
  `to_svg()` has no other effect.
- `const` — never mutates the figure.
- Re-renders from scratch on every call; no caching. For a figure that won't change again,
  it's harmless (if slightly wasteful) to call it more than once, but there's no need to
  — capture the result once if it's used more than once.
- The returned string is a *complete* standalone SVG document (with XML declaration), not
  a fragment — safe to write directly to a `.svg` file or embed as-is.

## See Also

- [`Studio::save_svg`](save_svg.md): `to_svg()` plus writing the result to a file.
- [`Studio::to_pdf`](to_pdf.md): the PDF equivalent.
- [`Studio::show`](show.md): wraps `to_svg()`'s output in an HTML viewer page and opens it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="subpane3.html">
    <span class="label">Previous</span>
    <span class="title">subpane3</span>
  </a>
  <a class="next-link" href="to_pdf.html">
    <span class="label">Next</span>
    <span class="title">to_pdf</span>
  </a>
</div>
