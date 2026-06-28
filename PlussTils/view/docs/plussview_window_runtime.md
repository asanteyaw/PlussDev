# PlussTilsView In-House Window Runtime

## Runtime contract

`PlussWebView` parses HTML, lays out the document, paints a software-rendered
pixel canvas, and presents that canvas in an operating-system window. It does
not delegate HTML rendering or scrolling to WebKit.

Select it through `HtmlWindowOptions::backend_preference`:

```cpp
pluss::view::HtmlWindowOptions options;
options.backend_preference = pluss::view::BackendPreference::PlussWebViewOnly;
pluss::view::show_html_window(html, options);
```

## Scrolling behavior

The in-house viewer exposes independent two-axis scroll regions for the
document and for every rendered table.

- A trackpad scroll preserves the platform's precise pixel delta.
- A wheel notch advances by a stable 40-pixel line step.
- The region under the pointer receives the input.
- When a nested table reaches an edge, unused movement continues into the
  document.
- `Shift` plus a vertical wheel scrolls horizontally.
- Horizontal and vertical scrollbar tracks support page clicks.
- Scrollbar thumbs support direct dragging.
- Dragging table or document content pans it on both axes.
- Table positions remain independent and survive temporary removal from the
  visible layout.
- Resizing clamps every offset to the new content boundary.

Keyboard commands are `Up`, `Down`, `Left`, `Right`, `Page Up`, `Page Down`,
`Home`, and `End`. Keyboard input targets the region beneath the most recent
pointer position and otherwise targets the document.

## Runtime components

- `Scene` stores renderer commands.
- `PixelCanvas` owns the RGBA frame.
- `SoftwareRenderer` rasterizes scenes with nested clipping.
- `ViewerRuntime` owns canvas configuration and frame submission.
- `HtmlScrollModel` owns floating-point offsets, nested routing, edge
  propagation, keyboard navigation, and pointer dragging.
- `IWindowShell` carries resize, pointer, text, and shortcut events.
- `CocoaWindowShell` is the current native shell implementation.

`HtmlRenderStats` is the contract between layout and input. It publishes
document overflow, table scroll regions, and track/thumb rectangles after
every layout. `HtmlScrollModel::update_metrics()` consumes those values and
clamps state before the next frame.

## Validation

Build and run the focused regression suite:

```sh
cmake --build build_cpp_env --target plussview_html_scroll_tests
ctest --test-dir build_cpp_env -R plussview_html_scroll_tests --output-on-failure
```

The suite covers horizontal and vertical document overflow, nested edge
propagation, multiple independent tables, anonymous table identity, precise
and notched input, Shift-wheel mapping, track clicks, thumb dragging, content
dragging, keyboard navigation, resize clamping, and visible frame changes.

The `plusstils_view_inhouse_scroll_showcase` target opens the real
`PlussWebViewOnly` Cocoa path with wide and tall document content plus two
independently scrollable tables.

## Remaining platform gaps

1. Linux and Windows still need in-house native shell adapters.
2. Text shaping and font fallback do not yet cover the full Unicode surface.
3. The software renderer is CPU-based; there is no GPU compositor.
