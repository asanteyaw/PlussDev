# PlussView API Reference

[Back to PlussTils API Reference](../api_reference.md) · [PlussView Manual](../manual/view.md)

If you want to show a `DataFrame`, an `Atlas`, or a plot, call `.show()` on
that object -- see [Showing a DataFrame, Atlas, or plot](view-showing-data.md)
for where that call actually is, and that type's own reference for its full
behavior. PlussView itself has no "show my data" call of its own: it is the
renderer those `.show()` methods use internally when they open a window
instead of printing text.

## Core rendering

The pipeline below is what that internal renderer -- "the built in renderer,"
in the codebase's own words -- is built from. Nothing outside PlussView's own
module calls any of it directly today; it is documented here because it is
real, public, and a genuine lower level underneath `.show()`, for pixel-level
control PlussView's own callers don't expose, not because it is a known,
separate way people build things with PlussView. The
[PlussView guide](../manual/view.md) teaches it at that level.

- [Types](view-types.md): `PointI`, `SizeI`, `RectI`, and `Color`.
- [`PixelCanvas`](view-canvas.md): the RGBA frame buffer.
- [`Scene` and the draw commands](view-scene.md): the retained command list and
  its fourteen command types.
- [`SoftwareRenderer`](view-renderer.md): applies a scene's commands to a canvas.
- [Export](view-export.md): writing a canvas to a PNG or PPM file.

## Not yet covered

Above the canvas/scene/renderer pipeline, PlussView has its own internal
document layout and scroll model, text layout, table and plot presentation, a
window-entry point, and a session/window-registry layer, under headers such
as `html_window.h`, `session.h`, `view_session.h`, `window_registry.h`,
`shell.h`, `table.h`, `plot.h`, `text_layout.h`, and the `html_*.h` headers.
Nothing outside PlussView's own module calls into any of this directly --
PlussBI, PlussMic's DataFrame display, and PlussPl's plotting render their own
HTML and open their own windows without constructing any of it -- so this
reference does not present it as a library to build plots or windows with by
hand. The [Window Runtime guide](../guides/view/window_runtime.md) describes
this internal layer at a conceptual level; it is PlussView's own
implementation detail, not a second public API.

```{toctree}
:maxdepth: 1
:hidden:

view-showing-data
view-types
view-canvas
view-scene
view-renderer
view-export
```
