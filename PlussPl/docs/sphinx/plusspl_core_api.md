# PlussPl Core API

`pluss::pl` is the low-level plotting bedrock. It exposes figure orchestration (`Studio`), pane layout (`Pane`), and trace styling (`Trace`).

## Key types

```cpp
namespace pluss::pl {
class Studio;
class Pane;
class Trace;
}
```

## Studio

Primary responsibilities:

- figure-level naming, headline, theme, extent
- subplot and nested subplot creation
- legend defaults and row titles
- export (`svg`, `pdf`, `png`) and interactive display

Core constructor:

```cpp
Studio(std::string name = {}, Theme theme = Theme::paper());
```

Common methods:

```cpp
Studio& rename(std::string name);
Studio& headline(std::string text);
Studio& theme(Theme theme);
Studio& size(double width, double height);
Studio& mosaic(std::int64_t rows, std::int64_t cols, double gutter_x = 18.0, double gutter_y = 18.0);
Studio& tight_layout(bool enable = true, double padding = 6.0);
Studio& legend(KeyDock dock);
Studio& legend_frame(KeyFrame frame);
Studio& legend_shape(KeyShape shape);
Studio& row_title(std::int64_t row, std::string text);

Pane& pane2(std::string title = {});
Pane& pane3(std::string title = {});
std::size_t pane_index(const Pane& pane) const;
Pane& subpane2(std::size_t parent_index, std::int64_t rows, std::int64_t cols,
               std::int64_t row, std::int64_t col,
               std::int64_t row_span = 1, std::int64_t col_span = 1,
               std::string title = {});
Pane& subpane3(std::size_t parent_index, std::int64_t rows, std::int64_t cols,
               std::int64_t row, std::int64_t col,
               std::int64_t row_span = 1, std::int64_t col_span = 1,
               std::string title = {});

std::string to_svg() const;
std::string to_pdf() const;
void save_svg(std::string_view path) const;
void save_pdf(std::string_view path, PdfEngine engine = PdfEngine::Auto) const;
void save_png(std::string_view path, std::int64_t dpi = 144) const;
bool show(std::string_view title = {}, std::int64_t width = 1280,
          std::int64_t height = 820, bool launch = true) const;
```

## Pane

Primary responsibilities:

- axis policies, tick format/labels, limits/scales, labels
- legends, grid visibility, cue overlays (rules/bands/pins)
- trace creation for 2D and 3D

High-traffic axis/style methods:

```cpp
Pane& x_label(std::string label);
Pane& y_label(std::string label);
Pane& z_label(std::string label);
Pane& x_limits(Range1d limits);
Pane& y_limits(Range1d limits);
Pane& z_limits(Range1d limits);
Pane& grid(GridStyle style);
Pane& legend(KeyPlace place);
Pane& legend_frame(KeyFrame frame);
Pane& legend_shape(KeyShape shape);
Pane& axes_visible(bool enable = true);
Pane& tick_font_scale(double scale);
Pane& axis_label_font_scale(double scale);
Pane& place(std::int64_t row, std::int64_t col);
Pane& place(std::int64_t row, std::int64_t col, std::int64_t row_span, std::int64_t col_span);
```

2D trace builders:

```cpp
trend, cloud, fieldmap, ribbon, spike, wedge, whisk,
columns, columns_h, bucket, quartet, violin, radial,
contour, isoband
```

3D trace builders:

```cpp
cloud3, path3, spire3, ridge3, isoshell, sheet, lattice
```

## Trace

Primary responsibilities:

- visual style control on emitted traces

Common methods:

```cpp
Trace& label(std::string label);
Trace& width(float width);
Trace& stroke(Rgba color);
Trace& palette(PaletteRamp palette);
Trace& color_limits(Range1d limits);
Trace& shade(ShadeMode mode);
Trace& lighting(LightingMode mode);
Trace& material(MaterialProfile profile);
Trace& transparent(bool enable = true);
Trace& annotate_levels(bool enable = true);
Trace& hide();
Trace& show();
```

## Enums and option types

Frequently used enums:

- `PaneKind`
- `GridStyle`
- `KeyPlace`, `KeyDock`, `KeyFrame`, `KeyShape`
- `ScaleKind`, `LimitMode`, `LimitMethod`, `TickFormat`
- `ProjectionKind`, `ShadeMode`, `LightingMode`, `MaterialProfile`
- `PdfEngine`
- violin-specific: `ViolinScale`, `ViolinInner`, `ViolinSide`

### KeyShape shorthand

A compact typed alias namespace is available:

```cpp
using namespace pluss::pl;

pane.legend_shape(ks::Horizontal);  // equivalent to KeyShape::Horizontal
```

## Minimal example

```cpp
#include <pluss/pl/studio.h>

using namespace pluss::pl;

Studio fig("core_demo", Theme::paper());
fig.mosaic(1, 2).headline("PlussPl core");

auto& left = fig.pane2("line");
left.place(0, 0).grid(GridStyle::Major).x_label("x").y_label("y");
left.trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 3, 2});

auto& right = fig.pane3("surface");
right.place(0, 1).grid(GridStyle::Major).x_label("x").y_label("y").z_label("z");
right.sheet(
    std::vector<double>{0, 1, 2},
    std::vector<double>{0, 1, 2},
    std::vector<double>{0, 1, 0, 1, 2, 1, 0, 1, 0});

fig.save_pdf("core_demo.pdf");
```
