# Flash Helper and Utility API

Additional user-facing `pluss::pl::flash` helpers that were not yet in the main plotting/style pages.

## API Reference Style

### `flash::axes_style`

#### Signature

```cpp
Pane& axes_style(self: Pane, style: GridStyle = GridStyle::Major);
Pane& axes_style(self: Pane, style: FlashStyle);
```

#### Notes

Alias helper for `set_style(...)` when you want axis-grid style changes without changing plotting calls.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/axes_style.cpp
:language: cpp
```

#### Output

```{only} html
![axes_style output](_static/plots/methods/axes_style.svg)
```

```{only} not html
![axes_style output](_static/plots/methods/axes_style.png)
```

### `flash::blend_palette`

#### Signature

```cpp
PaletteRamp blend_palette(colors: std::vector<Rgba>, name: std::string = "blend");
```

#### Notes

Builds a custom palette ramp from explicit color stops.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/blend_palette.cpp
:language: cpp
```

#### Output

```{only} html
![blend_palette output](_static/plots/methods/blend_palette.svg)
```

```{only} not html
![blend_palette output](_static/plots/methods/blend_palette.png)
```

### `flash::catplot_kind`

#### Signature

```cpp
CatPlotKind catplot_kind(kind: std::string_view);
```

#### Notes

Parses a categorical plot kind string (`strip`, `swarm`, `box`, `violin`, `bar`, `count`) into the typed enum used by `CatPlotOptions`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/catplot_kind.cpp
:language: cpp
```

#### Output

```{only} html
![catplot_kind output](_static/plots/methods/catplot_kind.svg)
```

```{only} not html
![catplot_kind output](_static/plots/methods/catplot_kind.png)
```

### `flash::crayon_palette`

#### Signature

```cpp
PaletteRamp crayon_palette();
```

#### Notes

Returns a fixed high-contrast crayon-like palette ramp.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/crayon_palette.cpp
:language: cpp
```

#### Output

```{only} html
![crayon_palette output](_static/plots/methods/crayon_palette.svg)
```

```{only} not html
![crayon_palette output](_static/plots/methods/crayon_palette.png)
```

### `flash::cubehelix_palette`

#### Signature

```cpp
PaletteRamp cubehelix_palette(n: std::int64_t = 6);
```

#### Notes

Returns a cubehelix-style palette ramp with controllable stop count.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/cubehelix_palette.cpp
:language: cpp
```

#### Output

```{only} html
![cubehelix_palette output](_static/plots/methods/cubehelix_palette.svg)
```

```{only} not html
![cubehelix_palette output](_static/plots/methods/cubehelix_palette.png)
```

### `flash::dark_palette`

#### Signature

```cpp
PaletteRamp dark_palette(color: Rgba, n: std::int64_t = 6);
```

#### Notes

Builds a ramp from black toward the supplied anchor color.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/dark_palette.cpp
:language: cpp
```

#### Output

```{only} html
![dark_palette output](_static/plots/methods/dark_palette.svg)
```

```{only} not html
![dark_palette output](_static/plots/methods/dark_palette.png)
```

### `flash::disgrid_kind`

#### Signature

```cpp
DisGridKind disgrid_kind(kind: std::string_view);
```

#### Notes

Parses distribution-grid mode (`hist`, `density`, `ecdf`, `rug`) into `DisGridKind`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/disgrid_kind.cpp
:language: cpp
```

#### Output

```{only} html
![disgrid_kind output](_static/plots/methods/disgrid_kind.svg)
```

```{only} not html
![disgrid_kind output](_static/plots/methods/disgrid_kind.png)
```

### `flash::diverging_palette`

#### Signature

```cpp
PaletteRamp diverging_palette(left: Rgba, right: Rgba, n: std::int64_t = 7);
```

#### Notes

Creates a left-white-right diverging palette for signed fields.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/diverging_palette.cpp
:language: cpp
```

#### Output

```{only} html
![diverging_palette output](_static/plots/methods/diverging_palette.svg)
```

```{only} not html
![diverging_palette output](_static/plots/methods/diverging_palette.png)
```

### `flash::flash_palette` and `flash::flash_palette_kind`

#### Signature

```cpp
FlashPalette flash_palette_kind(name: std::string_view);
PaletteRamp flash_palette(palette: FlashPalette = FlashPalette::Deep);
PaletteRamp flash_palette(name: std::string_view);
```

#### Notes

Maps style names (`deep`, `muted`, `pastel`, `bright`, `dark`, `colorblind`) to palette enums and ramps.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_palette_kind.cpp
:language: cpp
```

#### Output

```{only} html
![flash_palette_kind output](_static/plots/methods/flash_palette_kind.svg)
```

```{only} not html
![flash_palette_kind output](_static/plots/methods/flash_palette_kind.png)
```

### `flash::flash_palette` (direct)

#### Signature

```cpp
PaletteRamp flash_palette(palette: FlashPalette = FlashPalette::Deep);
PaletteRamp flash_palette(name: std::string_view);
```

#### Notes

Direct flash palette selection helper; equivalent output to `color_palette(...)` for flash palette names.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_palette.cpp
:language: cpp
```

#### Output

```{only} html
![flash_palette output](_static/plots/methods/flash_palette.svg)
```

```{only} not html
![flash_palette output](_static/plots/methods/flash_palette.png)
```

### `flash::flash_set`

#### Signature

```cpp
Studio& set(self: Studio, theme: Theme = Theme::paper());
```

#### Notes

Applies a base theme to the studio with a short helper call.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_set.cpp
:language: cpp
```

#### Output

```{only} html
![flash_set output](_static/plots/methods/flash_set.svg)
```

```{only} not html
![flash_set output](_static/plots/methods/flash_set.png)
```

### `flash::flash_show`

#### Signature

```cpp
bool show(self: Studio, title: std::string_view = {}, width: std::int64_t = 1280, height: std::int64_t = 820, launch: bool = true);
```

#### Notes

Displays a studio in the HTML viewer (in docs example, `launch=false` to keep capture deterministic).

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_show.cpp
:language: cpp
```

#### Output

```{only} html
![flash_show output](_static/plots/methods/flash_show.svg)
```

```{only} not html
![flash_show output](_static/plots/methods/flash_show.png)
```

### `flash::flash_save_png`

#### Signature

```cpp
void save_png(self: Studio, path: std::string_view, dpi: std::int64_t = 144);
```

#### Notes

Writes a raster PNG artifact from the current studio scene.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_save_png.cpp
:language: cpp
```

#### Output

```{only} html
![flash_save_png output](_static/plots/methods/flash_save_png.svg)
```

```{only} not html
![flash_save_png output](_static/plots/methods/flash_save_png.png)
```

### `flash::flash_theme`

#### Signature

```cpp
Theme flash_theme(style: FlashStyle = FlashStyle::Lightgrid, palette: FlashPalette = FlashPalette::Deep);
```

#### Notes

Constructs a theme object from flash style + palette presets.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/flash_theme.cpp
:language: cpp
```

#### Output

```{only} html
![flash_theme output](_static/plots/methods/flash_theme.svg)
```

```{only} not html
![flash_theme output](_static/plots/methods/flash_theme.png)
```

### `flash::get_dataset_names`

#### Signature

```cpp
Note get_dataset_names();
```

#### Notes

Returns names of built-in demo datasets available via `load_dataset(...)`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/get_dataset_names.cpp
:language: cpp
```

#### Output

```{only} html
![get_dataset_names output](_static/plots/methods/get_dataset_names.svg)
```

```{only} not html
![get_dataset_names output](_static/plots/methods/get_dataset_names.png)
```

### `flash::hls_palette`

#### Signature

```cpp
PaletteRamp hls_palette(n: std::int64_t = 6, saturation: double = 0.68, lightness: double = 0.52);
```

#### Notes

Programmatic HLS wheel palette builder.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/hls_palette.cpp
:language: cpp
```

#### Output

```{only} html
![hls_palette output](_static/plots/methods/hls_palette.svg)
```

```{only} not html
![hls_palette output](_static/plots/methods/hls_palette.png)
```

### `flash::husl_palette`

#### Signature

```cpp
PaletteRamp husl_palette(n: std::int64_t = 6);
```

#### Notes

HUSL-like palette shortcut.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/husl_palette.cpp
:language: cpp
```

#### Output

```{only} html
![husl_palette output](_static/plots/methods/husl_palette.svg)
```

```{only} not html
![husl_palette output](_static/plots/methods/husl_palette.png)
```

### `flash::light_palette`

#### Signature

```cpp
PaletteRamp light_palette(color: Rgba, n: std::int64_t = 6);
```

#### Notes

Builds a ramp from white toward the supplied anchor color.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/light_palette.cpp
:language: cpp
```

#### Output

```{only} html
![light_palette output](_static/plots/methods/light_palette.svg)
```

```{only} not html
![light_palette output](_static/plots/methods/light_palette.png)
```

### `flash::make_dataset`

#### Signature

```cpp
Dataset make_dataset(numeric_values: std::vector<Series>, numeric_names: std::vector<std::string> = {});
Dataset make_dataset(matrix: Series, feature_names: std::vector<std::string> = {}, feature_axis: Axis = Axis::Cols);
```

#### Notes

Creates a typed flash dataset from either explicit feature vectors or a feature matrix.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/make_dataset.cpp
:language: cpp
```

#### Output

```{only} html
![make_dataset output](_static/plots/methods/make_dataset.svg)
```

```{only} not html
![make_dataset output](_static/plots/methods/make_dataset.png)
```

### `flash::move_legend`

#### Signature

```cpp
Pane& move_legend(self: Pane, place: KeyPlace) noexcept;
Studio& move_legend(self: Studio, dock: KeyDock) noexcept;
```

#### Notes

Repositions legend per-pane or globally per-studio.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/move_legend.cpp
:language: cpp
```

#### Output

```{only} html
![move_legend output](_static/plots/methods/move_legend.svg)
```

```{only} not html
![move_legend output](_static/plots/methods/move_legend.png)
```

### `flash::mpl_palette`

#### Signature

```cpp
PaletteRamp mpl_palette(name: std::string_view = "viridis");
```

#### Notes

Matplotlib-style palette compatibility helper.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/mpl_palette.cpp
:language: cpp
```

#### Output

```{only} html
![mpl_palette output](_static/plots/methods/mpl_palette.svg)
```

```{only} not html
![mpl_palette output](_static/plots/methods/mpl_palette.png)
```

### `flash::relgrid_kind`

#### Signature

```cpp
RelGridKind relgrid_kind(kind: std::string_view);
```

#### Notes

Parses relation-grid mode (`scatter`, `line`, `reg`) into `RelGridKind`.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/relgrid_kind.cpp
:language: cpp
```

#### Output

```{only} html
![relgrid_kind output](_static/plots/methods/relgrid_kind.svg)
```

```{only} not html
![relgrid_kind output](_static/plots/methods/relgrid_kind.png)
```

### `flash::tickless`

#### Signature

```cpp
Pane& tickless(self: Pane, enable: bool = true);
```

#### Notes

Suppresses axis framing while keeping data traces visible.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/tickless.cpp
:language: cpp
```

#### Output

```{only} html
![tickless output](_static/plots/methods/tickless.svg)
```

```{only} not html
![tickless output](_static/plots/methods/tickless.png)
```

### `flash::xkcd_palette`

#### Signature

```cpp
PaletteRamp xkcd_palette();
```

#### Notes

Returns a fixed playful palette ramp.

#### Example

```{literalinclude} ../../examples/docs/snippets/keys/xkcd_palette.cpp
:language: cpp
```

#### Output

```{only} html
![xkcd_palette output](_static/plots/methods/xkcd_palette.svg)
```

```{only} not html
![xkcd_palette output](_static/plots/methods/xkcd_palette.png)
```
