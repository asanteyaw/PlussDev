# Pane API Reference

This page documents the full public `pluss::pl::Pane` surface — the 2D/3D plotting area
that lives inside a [`Studio`](studio_api_reference.md). A `Studio` owns a collection of
`Pane`s; each `Pane` owns its own axes (scale, limits, ticks, labels, grid), its own
legend/caption/cue decorations, its own alignment/visibility/placement state, and the
`Trace`s drawn into it by the trace-creation methods (`trend`, `cloud`, `columns`,
`bucket`, `quartet`, `wedge`, `violin`, `radial`, `whisk`, `ribbon`, `spike`, `contour`,
`fieldmap`, and their `Volumetric`-only 3D counterparts `cloud3`, `path3`, `spire3`,
`isoband`, `ridge3`, `isoshell`, `sheet`, `lattice`).

Get a `Pane` from [`Studio::pane2`](api/studio/pane2.md) (2D, `Planar`) or
[`Studio::pane3`](api/studio/pane3.md) (3D, `Volumetric`) — or
[`Studio::subpane2`](api/studio/subpane2.md) / [`Studio::subpane3`](api/studio/subpane3.md)
for a pane nested inside a mosaic cell. Every trace-creation method enforces its own
`Planar`-vs-`Volumetric` requirement — calling a 3D-only method on a `pane2()` (or a
2D-only method on a `pane3()`) throws `std::logic_error` immediately, before any rendering
is attempted.

```{important} Pane& references are invalidated by later pane-creation calls
`Studio` stores its panes in a single `std::vector<Pane>` internally. A `Pane&` returned by
`pane2()`, `pane3()`, `subpane2()`, or `subpane3()` is invalidated the moment another such
call is made on the same `Studio` — exactly like any reference into a `std::vector` that
gets reallocated. Capture any value you need from an earlier pane (a label, an alignment
group name, a visibility flag) into a local variable *immediately* after creating that
pane, before creating the next one. This bit multiple probes written during this
documentation effort and is the single most common `Pane`-related mistake to guard
against.
```

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll
  of every method.

## Organization

The full method surface (108 pages) is organized, in reading order via each page's
Previous/Next links, into these groups:

- **Identity, title, theme** — `kind`, `title_text`, `title`, `theme_tokens`.
- **Camera and lighting** (3D) — `lens`, `lights`.
- **Axis scale and limits** — `scale_x`/`y`/`z`, `x`/`y`/`z_limits`, `color_limits`,
  `x`/`y`/`z_limit_method`, `color_limit_method`.
- **Axis labels and grid** — `axis_accessors`, `x`/`y`/`z_label`, `grid`,
  `x`/`y`/`z_major_grid`, `x`/`y`/`z_minor_grid`.
- **Tick formatting** — `x`/`y`/`z_tick_format`, `x`/`y`/`z_auto_tick_decimals`,
  `x`/`y`/`z_tick_rotation`, `x`/`y`/`z_tick_label_gap`, `x`/`y`/`z_label_gap`,
  `x`/`y`/`z_label_rotation`, `x`/`y`/`z_tick_labels`, `y_label_centered`,
  `x`/`y`/`z_ticks`, `x`/`y`/`z_categories`.
- **Legend** — `legend`, `legend_place`, `legend_frame`, `legend_shape`, `scale_guide`.
- **Caption and cues** — `caption`, `caption_text`, `rule_x`/`y`/`z`, `band_x`/`y`/`z`,
  `pin`, `rules`, `bands`, `pins`.
- **Alignment** — `align_x`/`y`/`z`, `align_xy`, `align_xyz`, `clear_align`,
  `align_group_accessors`.
- **Visibility, placement, font scale** — `visible`, `axes_visible`,
  `parallel_xy_labels`, `place`, `tick_font_scale`, `axis_label_font_scale`.
- **2D trace creation** — `trend`, `cloud`, `columns`, `columns_h`, `bucket`, `quartet`,
  `wedge`, `violin`, `radial`, `whisk`, `ribbon`, `spike`, `contour`, `fieldmap`,
  `isoband` (filled contours, still `Planar`).
- **3D trace creation** (`Volumetric` only) — `cloud3`, `path3`, `spire3`, `ridge3`
  (elevated contours), `isoshell`, `sheet`, `lattice`.

```{important} Fixed: visible(), axes_visible(), parallel_xy_labels() no longer default enable
Each of these three names used to declare a setter with a default argument
(`bool enable = true`) alongside a separate `const`-qualified getter of the same name,
which meant a bare zero-argument call like `pane.visible()` on a non-`const` `Pane&`
silently resolved to the **setter** rather than reading the value — directly confirmed by
compiled reproduction during this documentation effort. All three have since been fixed at
the source: `enable` is now a required argument, so a bare zero-argument call always
resolves to the `const` getter, on both `const` and non-`const` panes. See
[`visible`](api/pane/visible.md), [`axes_visible`](api/pane/axes_visible.md), and
[`parallel_xy_labels`](api/pane/parallel_xy_labels.md) for the full, verified detail.
```

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly
to a method name.

```{toctree}
:hidden:
:maxdepth: 1

api/pane/align_group_accessors
api/pane/align_x
api/pane/align_xy
api/pane/align_xyz
api/pane/align_y
api/pane/align_z
api/pane/axes_visible
api/pane/axis_accessors
api/pane/axis_label_font_scale
api/pane/band_x
api/pane/band_y
api/pane/band_z
api/pane/bands
api/pane/bucket
api/pane/caption
api/pane/caption_text
api/pane/clear_align
api/pane/cloud
api/pane/cloud3
api/pane/color_limit_method
api/pane/color_limits
api/pane/columns
api/pane/columns_h
api/pane/contour
api/pane/fieldmap
api/pane/grid
api/pane/isoband
api/pane/isoshell
api/pane/kind
api/pane/lattice
api/pane/legend
api/pane/legend_frame
api/pane/legend_place
api/pane/legend_shape
api/pane/lens
api/pane/lights
api/pane/parallel_xy_labels
api/pane/path3
api/pane/pin
api/pane/pins
api/pane/place
api/pane/quartet
api/pane/radial
api/pane/ribbon
api/pane/ridge3
api/pane/rule_x
api/pane/rule_y
api/pane/rule_z
api/pane/rules
api/pane/scale_guide
api/pane/scale_x
api/pane/scale_y
api/pane/scale_z
api/pane/sheet
api/pane/spike
api/pane/spire3
api/pane/theme_tokens
api/pane/tick_font_scale
api/pane/title
api/pane/title_text
api/pane/trend
api/pane/violin
api/pane/visible
api/pane/wedge
api/pane/whisk
api/pane/x_auto_tick_decimals
api/pane/x_categories
api/pane/x_label
api/pane/x_label_gap
api/pane/x_label_rotation
api/pane/x_limit_method
api/pane/x_limits
api/pane/x_major_grid
api/pane/x_minor_grid
api/pane/x_tick_format
api/pane/x_tick_label_gap
api/pane/x_tick_labels
api/pane/x_tick_rotation
api/pane/x_ticks
api/pane/y_auto_tick_decimals
api/pane/y_categories
api/pane/y_label
api/pane/y_label_centered
api/pane/y_label_gap
api/pane/y_label_rotation
api/pane/y_limit_method
api/pane/y_limits
api/pane/y_major_grid
api/pane/y_minor_grid
api/pane/y_tick_format
api/pane/y_tick_label_gap
api/pane/y_tick_labels
api/pane/y_tick_rotation
api/pane/y_ticks
api/pane/z_auto_tick_decimals
api/pane/z_categories
api/pane/z_label
api/pane/z_label_gap
api/pane/z_label_rotation
api/pane/z_limit_method
api/pane/z_limits
api/pane/z_major_grid
api/pane/z_minor_grid
api/pane/z_tick_format
api/pane/z_tick_label_gap
api/pane/z_tick_labels
api/pane/z_tick_rotation
api/pane/z_ticks
```
