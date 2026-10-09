# PlussView Architecture

[Back to the PlussView manual](../../manual/view.md)

PlussView is Pluss's built-in renderer: the engine that opens a window and
paints into it when `DataFrame::show()`, `Atlas::show()`, or PlussPl's
`Studio::show()` decide to open one instead of printing text. It is
independent of the Core and Loader shelves, but it is not a standalone
utility those callers reach into by hand -- it is their implementation.

## Boundaries

1. `DataFrame`, `Atlas`, and PlussPl's `Studio`/`flash` render their own HTML
   or build their own `.show()` presentation, and choose a backend through
   their own options types -- never constructing anything below this line
   themselves.
2. `pluss::view::show_html_window` is the one entry point those callers
   reach: it opens a window (native or in-house) for a complete HTML
   document, or for a caller's own `VirtualTableModel`.
3. `pluss::view::window` is everything underneath `show_html_window` when
   the in-house backend is chosen: HTML layout and scrolling, scene
   construction, rasterization, platform shells, and session/registry
   ownership. Nothing outside this module constructs any of it directly
   today -- it is PlussView's own implementation, not a second public API,
   confirmed by its one real git history entry: `fixed the scrolling on the
   built in renderer`.
4. The canvas/scene/renderer/export pipeline inside `pluss::view::window` is
   real and public (documented on its [own reference pages](../../api/view.md)),
   for whoever wants pixel-level control without a `DataFrame` at all -- but
   it has no known caller outside PlussView's own module either.

## Source map

| Concern | Location |
|---|---|
| Public umbrella | `PlussTils/include/pluss/tils/view.h` |
| Internal renderer (canvas/scene/renderer/export are public; the rest is not) | `PlussTils/include/pluss/tils/view/window/` |
| Session and registry ownership | `PlussTils/view/src/window/core/` |
| Scene construction | `PlussTils/view/src/window/scene/` |
| HTML layout and scrolling | `PlussTils/view/src/window/html/` |
| Rasterization and text | `PlussTils/view/src/window/render/` |
| Platform shells | `PlussTils/view/src/window/shell/` and `src/platform/` |
| Export | `PlussTils/view/src/window/export/` |

The API reference documents public contracts. This guide explains boundaries;
it does not make internal classes public by naming their subsystem.
