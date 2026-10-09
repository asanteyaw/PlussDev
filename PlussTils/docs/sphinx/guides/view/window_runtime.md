# PlussView Window Runtime

[Back to the PlussView manual](../../manual/view.md)

The in-house runtime lays out supported documents, paints a software canvas,
and presents the resulting frame through a native window shell. It does not
delegate its layout or scrolling model to an embedded browser engine.

## Input routing

Document and table regions can scroll independently on both axes. Input is
routed to the region under the pointer; unused movement propagates outward when
a nested region reaches an edge. Precise trackpad deltas remain precise, wheel
notches use the configured line step, and Shift maps vertical wheel input to
horizontal movement.

Scrollbar tracks support page movement, thumbs support direct dragging, and
content dragging pans the active region. Resize events clamp offsets against
the new content bounds. Keyboard movement includes arrows, page movement,
`Home`, and `End`, targeting the most recently addressed region or the document
when no nested region is active.

## Runtime responsibilities

| Component | Responsibility |
|---|---|
| `Scene` | Retains renderer commands and their ordering |
| `PixelCanvas` | Owns the RGBA destination frame |
| `SoftwareRenderer` | Rasterizes scenes under nested clipping |
| `ViewerRuntime` | Coordinates canvas configuration and frame submission |
| `HtmlScrollModel` | Retains offsets and resolves nested pointer or keyboard movement |
| `IWindowShell` | Carries resize, pointer, text, and shortcut events across the host boundary |

The public reference will specify which of these contracts are stable public
types and which are implementation details before presenting call-level pages.
