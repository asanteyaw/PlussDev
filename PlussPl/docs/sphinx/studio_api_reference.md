# Studio API Reference

This page documents the full public `pluss::pl::Studio` surface — the top-level figure
object in PlussPl, the plotting and visualization layer of Pluss. A `Studio` owns a
collection of `Pane`s (each holding `Trace`s drawn by the `flash::` plotting functions),
the figure's overall theme and layout, and every export/display path (`to_svg`, `to_pdf`,
`save_svg`, `save_pdf`, `save_png`, `show`).

Start with [`pluss::pl::studio`](api/studio/studio.md) — the idiomatic entry point that
builds an empty `Studio` — then [`pane2`](api/studio/pane2.md) /
[`pane3`](api/studio/pane3.md) to add somewhere to plot into.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified
  outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll
  of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly
to a method name.

```{toctree}
:hidden:
:maxdepth: 1

api/studio/clear_pane_style_defaults
api/studio/clear_row_title
api/studio/clear_row_titles
api/studio/context_scales
api/studio/extent
api/studio/headline
api/studio/headline_text
api/studio/legend
api/studio/legend_dock
api/studio/legend_frame
api/studio/legend_shape
api/studio/mosaic
api/studio/mosaic_layout
api/studio/name
api/studio/pane2
api/studio/pane3
api/studio/pane_count
api/studio/pane_index
api/studio/pane_style_defaults
api/studio/panes
api/studio/rename
api/studio/row_title
api/studio/row_titles
api/studio/save_pdf
api/studio/save_png
api/studio/save_svg
api/studio/show
api/studio/size
api/studio/studio
api/studio/style
api/studio/style_options
api/studio/subpane2
api/studio/subpane3
api/studio/theme
api/studio/theme_tokens
api/studio/tight_layout
api/studio/tight_layout_enabled
api/studio/tight_layout_padding
api/studio/to_pdf
api/studio/to_svg
api/studio/x_caption
api/studio/x_captions
api/studio/y_caption
api/studio/y_captions
api/studio/z_caption
api/studio/z_captions
```
