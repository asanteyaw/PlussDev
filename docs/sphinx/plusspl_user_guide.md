# PlussPl User Guide

PlussPl is the core plotting surface built around `pluss::pl::Studio` and `pluss::pl::Pane`.

## API Signature

- `#include <pluss/pl.h>`
- `pl::studio(name: string = "") -> pl::Studio`
- `Studio::pane2(title: string = "") -> Pane&`
- `Studio::pane3(title: string = "") -> Pane&`

## Notes

Use PlussPl directly when you want low-level plot construction, full control over panes/traces, and explicit export/display management.

## Inputs

- Numeric vectors / `Series`
- Optional titles, layout values, and theme controls

## Output

- In-memory `Studio` graph object
- Exportable SVG/PDF/PNG output

## Options

- `Theme` (`Theme::paper()`, `Theme::ocean()`)
- `GridStyle`, `KeyPlace`, `KeyDock`, `KeyFrame`, `KeyShape`
- `PdfEngine` when saving PDF

## Examples

### 1) Minimal 2D line chart

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("minimal_2d");
auto& p = fig.pane2("Trend");
p.trend(std::vector<double>{0, 1, 2, 3}, std::vector<double>{1, 4, 9, 16}).title("x^2");
fig.headline("Minimal 2D");
fig.save_svg("minimal_2d.svg");
```

### Expected output

- One 2D pane containing a single line trace titled `x^2`
- SVG written to `minimal_2d.svg`

### 2) Mosaic with aligned panes

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("mosaic");
fig.mosaic(1, 2).tight_layout(true, 6.0);

auto& left = fig.pane2("Left");
auto& right = fig.pane2("Right");

left.trend(std::vector<double>{1, 2, 3}, std::vector<double>{2, 4, 6});
right.cloud(std::vector<double>{1, 2, 3}, std::vector<double>{2, 3, 5});

left.align_y("shared_y");
right.align_y("shared_y");

fig.save_pdf("mosaic.pdf");
```

### Expected output

- Two panes in a 1x2 layout
- Shared y-scale alignment across panes
- PDF written to `mosaic.pdf`

### 3) 3D pane

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("volumetric");
auto& p3 = fig.pane3("Cloud3");
p3.cloud3(
    std::vector<double>{0, 1, 2, 3},
    std::vector<double>{0, 1, 0, 1},
    std::vector<double>{1, 3, 2, 4})
  .title("3D points");
fig.save_png("cloud3.png", 180);
```

### Expected output

- One 3D pane with a cloud trace
- PNG written to `cloud3.png`

### 4) Display window

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("preview");
fig.pane2("Line").trend(std::vector<double>{0, 1, 2}, std::vector<double>{1, 2, 1});
const bool ok = fig.show("PlussPl Preview", 1200, 760, true);
```

### Expected output

- A local preview window/html render opens when supported
- `ok == true` when rendering succeeds

## Common workflow

1. Create `Studio`
2. Add `pane2()`/`pane3()`
3. Add traces (`trend`, `cloud`, `fieldmap`, `sheet`, `lattice`, ...)
4. Style/layout (`theme`, `mosaic`, `tight_layout`, legend/axis settings)
5. Export (`save_svg`, `save_pdf`, `save_png`) or `show`
