<!-- pluss-hand-authored -->

# Studio::show

[Back to Studio API Reference](../../studio_api_reference)

## Signatures

```cpp
bool Studio::show(std::string_view title = {}, std::int64_t width = 1280,
                   std::int64_t height = 820, bool launch = true) const

bool Studio::show(const pluss::view::HtmlWindowOptions& options, bool launch = true) const
```

## What It Does

Opens the figure in an interactive viewer window — this is the everyday "look at my plot"
method used throughout every other example in this documentation set. Both overloads
render the figure via [`to_svg()`](to_svg.md), wrap the SVG in a small self-contained HTML
page (with a toolbar; see the source for exact styling), and hand that HTML to the host
platform's window-launching machinery.

- The first overload takes a plain `title`/`width`/`height` — the common case. An empty
  `title` (the default) falls back to the `Studio`'s own [`name()`](name.md), or
  `"PlussPl Plot"` if that's empty too.
- The second overload takes a full `pluss::view::HtmlWindowOptions` (from `PlussTils`) for
  finer control — a custom snapshot filename, backend preference, and other window-hosting
  details beyond title/width/height.

Both overloads return `bool`: `true` if a window was launched (or, with `launch=false`,
if the fallback HTML file was written successfully), `false` if launching failed and even
the fallback write couldn't complete.

```{important} Examples 2-4 pass launch=false -- useful for testing the return value, not typical usage
Examples 2, 3, and 4 below pass `launch=false` explicitly, specifically to make
`show()`'s `bool` return value and its second (`HtmlWindowOptions`) overload
demonstrable in an automated, non-interactive environment without popping up a
window. This is **not** how `show()` is normally called — ordinary usage (Example
1) omits `launch` entirely (it defaults to `true`) and lets a real window open,
exactly like every other `.show()` call throughout this documentation set. Even
with `launch=false`, `show()` still renders the figure and writes a fallback HTML
file to a temp directory — it just skips the final step of actually opening it in
a viewer.
```

## Parameters

**First overload:**

**title** (`std::string_view`, default `{}`)
    The window/page title. Empty falls back to the figure's `name()`, then to
    `"PlussPl Plot"`.

**width**, **height** (`std::int64_t`, default `1280`, `820`)
    The requested viewer window size in pixels. Internally clamped to a `320`x`240`
    minimum.

**launch** (`bool`, default `true`)
    Whether to actually open the viewer window. `false` still renders and writes a
    fallback HTML file, but never launches anything — see the important callout above.

**Second overload:**

**options** (`const pluss::view::HtmlWindowOptions&`)
    Full window-hosting configuration from `PlussTils`, including `title`, `width`,
    `height`, snapshot filenames, and backend preference. `title`/`width`/`height` are
    read from `options` the same way as the first overload's direct parameters (with a
    `640`x`420` minimum instead of `320`x`240`).

**launch** (`bool`, default `true`)
    Same meaning as the first overload.

## Returns

`bool`
    `true` if a window was launched, or (with `launch=false`) if the fallback HTML file
    was written; `false` if even that failed.

## Examples

Example 1. Typical usage — this is what real, interactive code looks like: bare
`fig.show();`, `launch` left at its default `true`, exactly like every other `.show()`
call throughout this documentation set.

```cpp
using namespace pluss::pl;

auto fig = studio("my_figure");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
fig.show();
```

![Output of Example 1](img/to_svg-1-basic.svg)

Example 2. The first overload with an explicit title and size, `launch=false` for
automated verification.

```cpp
using namespace pluss::pl;

auto fig = studio("show_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

bool result = fig.show("Custom Title", 1600, 900, /*launch=*/false);
std::cout << result << "\n";
```

### Output

```text
1
```

![Output of Example 2](img/to_svg-1-basic.svg)

Example 3. An empty title falls back to the figure's own `name()`.

```cpp
using namespace pluss::pl;

auto fig = studio("show_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

bool result = fig.show(/*title=*/{}, 1280, 820, /*launch=*/false);
std::cout << result << "\n";
```

### Output

```text
1
```

![Output of Example 3](img/to_svg-1-basic.svg)

Example 4. The `HtmlWindowOptions` overload, for finer-grained window configuration.

```cpp
using namespace pluss::pl;

auto fig = studio("show_check");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});

pluss::view::HtmlWindowOptions options;
options.title = "Advanced Window";
options.width = 1440;
options.height = 960;

bool result = fig.show(options, /*launch=*/false);
std::cout << result << "\n";
```

### Output

```text
1
```

![Output of Example 4](img/to_svg-1-basic.svg)

## Notes

- Not `noexcept`.
- The `title`/`width`/`height` overload is the one used everywhere else in this
  documentation set as bare `.show()` — the common case.
- `launch=false` is a testing/automation affordance, not typical usage — see the important
  callout above.
- Renders via `to_svg()` internally, so `show()` always reflects the figure's current
  state at call time, same as `to_svg()` itself. `title`/`width`/`height`/`options` only
  affect the *wrapping* HTML page, never the SVG payload itself — directly verified: all
  four examples above render byte-identical SVGs despite differing titles/sizes/overloads.
- Unlike `save_svg()`/`save_pdf()`/`save_png()`, `show()` never writes a *permanent* file
  the caller chose the path for — any HTML file it writes (for the fallback path, or as
  part of `launch=true`'s window-hosting machinery) lives in a temp directory.

## See Also

- [`Studio::to_svg`](to_svg.md): what `show()` renders internally before wrapping it in
  HTML.
- [`Studio::save_svg`](save_svg.md) / [`save_pdf`](save_pdf.md) / [`save_png`](save_png.md):
  persist the figure to a caller-chosen file path instead of opening a viewer.
- [`Studio::name`](name.md): the fallback title source when `title`/`options.title` is
  empty.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_png.html">
    <span class="label">Previous</span>
    <span class="title">save_png</span>
  </a>
  <a class="next-link" href="panes.html">
    <span class="label">Next</span>
    <span class="title">panes</span>
  </a>
</div>
