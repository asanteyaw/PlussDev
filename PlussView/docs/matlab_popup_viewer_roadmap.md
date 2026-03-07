# MATLAB-Informed Roadmap: Cross-Platform Popup Viewer (In-House HTML Renderer)

## Scope
Build a **Pluss-owned popup viewer stack** that is cross-platform and consistent across PlussPl/PlussMic/other libraries.

This roadmap uses MATLAB architecture cues from readable `.m` files in the R2025b install and adapts them to our C++20 stack.

## Pivot Decision (2026-03-06)
We will run a **two-phase strategy** with **in-house ownership of HTML rendering**:

1. **Phase 1 (Primary): In-house HTML renderer runtime**
- Parse and render HTML/CSS-like content directly in `PlussView` runtime.
- The in-house engine owns rendering for:
  - text
  - layout
  - buttons
  - charts
  - tables
  - SVG graphics
  - Canvas graphics
  - WebGL-style graphics (backend-owned, no external browser dependency)

2. **Phase 2 (Secondary): Browser-host debug/reference path**
- Keep `WKWebView`/`WebView2`/`WebKitGTK` only as optional compatibility tooling (debug/reference snapshots), not as the product rendering engine.

## Clarification
The product goal is **not** "send HTML to OS webview and let it render."
The goal is: **PlussView receives plot/table/view data, builds HTML-like document + style model, and renders it itself.**

## MATLAB Files Reviewed
I reviewed these concrete files (plus related templates/assets):

- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/FigurePlatformHostFactory.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/FigurePlatformHost.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/CEFFigurePlatformHost.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/DivFigurePlatformHost.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/EmbeddedFigurePlatformHost.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/+platformhost/MOFigurePlatformHost.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/FigureController.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/FigureServices.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uicomponents/components/+matlab/+ui/+internal/+controller/FigurePeerModelInfo.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/cefclient/+matlab/+internal/webwindow.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/cefclient/+matlab/+internal/webwindowmanager.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/connector2/webwindow/m/+connector/+internal/webwindow.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/connector2/webwindow/m/+connector/+internal/webwindowmanager.m`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uifigureappjs/cefComponentContainer.html`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uifigureappjs/webAppsComponentContainer.html`
- `/Applications/MATLAB_R2025b.app/toolbox/matlab/uitools/uifigureappjs/moComponentContainer.html`

Note: many internals in the MATLAB tree are `.p` compiled artifacts and are not inspectable directly.

## What MATLAB Pattern We Should Copy
1. **Host Factory**
- Environment-aware host selection (`FigurePlatformHostFactory`).
- One controller path, multiple host implementations.

2. **Controller-to-Host Delegation**
- Central controller (`FigureController`) owns behavior updates.
- Host handles only platform mechanics: window, focus, visibility, resize, window state, export wiring.

3. **Message/Channel Contract**
- Explicit client-server channels and event contracts (`FigureServices`, `DivFigurePlatformHost`, `EmbeddedFigurePlatformHost`).
- Handshake/ready event before expensive sync.

4. **Lifecycle Registry**
- Global manager tracks windows and cleanup (`webwindowmanager`, lifecycle managers).

5. **Separate Render Surface from Windowing**
- HTML/JS surface is separate from host transport.
- Multiple containers can host same logical figure surface.

## Target PlussView Architecture (In-House HTML Runtime)
1. `SessionController`
- Owns per-window lifecycle and state transitions.

2. `HtmlDocumentModel`
- Internal DOM-like tree (`Node`, attributes, text runs, component nodes for chart/table).

3. `StyleSystem`
- CSS-like style resolution (cascading, inheritance, selectors subset, variables subset).

4. `LayoutEngine`
- Deterministic layout for block/inline/flex/grid/table primitives.

5. `WindowShellAdapter`
- OS shell bindings for window lifecycle/input/presentation only.
- No browser engine delegation for rendering.

6. `ExportService`
- Unified PNG/SVG/PDF export from in-house render tree/frame outputs.

7. `WindowRegistry`
- Track live sessions, parent/child relationships, cleanup guarantees.

8. `PolicyLayer`
- Default sizing, docking strategy, input routing, DPI policy.

9. `RenderBackends`
- `SoftwareRenderEngine` (current default)
- future `GpuRenderEngine` (accelerated raster/vector/3D)

10. `Script/Interaction Layer`
- Command bridge for controls and interactions (button click, zoom, pan, table paging) without requiring external browser JS runtime.

## Current Status (as of now)
- We already have centralization started in `PlussView`.
- PlussPl and PlussMic now delegate popup creation to `PlussView`.
- Linux GTK/WebKit host has been migrated into `PlussView`.
- macOS and Windows native hosts are in `PlussView`.
- `SessionController` lifecycle state machine is now in place.
- Backend detection is now side-effect free (no probe-launch calls).
- Host capability introspection (`detect_native_backend`, `native_host_capabilities`) is now available for upper layers.
- In-house viewer runtime scaffold now exists (`pluss::view::window`) with:
  - scene command model
  - software pixel canvas
  - software renderer abstraction
  - math-aware text layout + raster text rendering
  - line-plot scene composition API
  - table scene composition API with deterministic pagination window
  - typed line-plot interaction controller (`zoom` / `pan` / `reset`) and active view-window state
  - table interaction controller (`next page` / `previous page` / `set offset`)
  - `LinePlotWindowSession` command/event loop (controller + runtime + shell integration) with deterministic command queue
  - `TableWindowSession` command/event loop using the same in-house runtime contract
  - `WindowRegistry` singleton for multi-session lifecycle management (register/open/enqueue/tick/close/prune/snapshot)
  - shell interaction protocol events (`ShortcutCommand`, `PointerScroll`) mapped to in-house window commands
  - configurable input policy in session config (key pan step, key zoom factor, scroll zoom factor, scroll inversion)
  - session/registry backend introspection (`Null` vs native shell backend) for deterministic deployment checks
  - shell capability contract (`supports_live_present`, resize/shortcut/scroll support) exposed via session snapshots
  - demo shell policy now defaults to native-preferred and manual close (`x`) with optional timed override for automation (`PLUSSVIEW_DEMO_HOLD_MS`)
  - in-house canvas export service with strict format/extension contract (`.png`, `.ppm`) and registry-level snapshot export
  - runtime and shell state/event models
  - shell factory + native Cocoa shell adapter (macOS, image/canvas present path)
  - first standalone runtime and first-plot demo targets
  - interaction-core demo target that deterministically emits zoom/pan/reset frame snapshots
  - native popup showcase target is now explicitly treated as experimental and disabled by default

The Cocoa shell path currently presents software-rendered frames via `NSImageView`; this confirms the shell abstraction works.
The remaining work is to make the in-house renderer HTML-capable (document/style/layout/render pipeline), not to outsource rendering to OS webviews.

This keeps momentum high while preserving optional long-term in-house rendering control.

## Patch Estimate (In-House HTML Renderer)
Definition: in-house HTML/CSS-like render path, with browser hosts used only as optional reference tooling.

- Phase 1 (in-house HTML MVP): **14-22 patches**
- Phase 2 (performance + feature hardening): **16-26 patches**
- Confidence: medium (layout/styling feature boundaries must stay disciplined)

Current status estimate:
- Core runtime/shell scaffolding progress: high
- Remaining to first full in-house HTML interactive popup across desktop targets: moderate-high

Compression approach:
1. Bundle document model + style resolution together.
2. Bundle layout engine + render tree emission together.
3. Bundle chart/table component renderer + interaction routing together.
4. Bundle export + deterministic visual regression testing together.

## Acceptance Criteria for “Mature Popup Viewer”
1. Same behavior and shortcuts on macOS/Windows/Linux.
2. Same default layout metrics (margins, titlebar spacing, toolbar placement).
3. Deterministic export behavior and naming.
4. Clean lifecycle: no orphan windows/channels.
5. Robust headless fallback without crashing.
6. Integration tests for PlussPl plots and PlussMic table displays.
7. In-house renderer can render text/layout/buttons/charts/tables/SVG/Canvas/WebGL-style scenes across all supported desktop hosts.
8. Same content produces equivalent layout output across macOS/Windows/Linux with deterministic tests.

## Proposed Immediate Next 5 Patches
1. Define `HtmlDocumentModel` + style schema and map `LinePlotWindowSession`/`TableWindowSession` data into document nodes.
2. Implement deterministic layout core (block/inline/flex/table subset) with text metrics integration.
3. Implement render tree pass for text, boxes, borders, SVG primitives, canvas primitives, and chart/table widgets.
4. Add interaction dispatch for buttons, zoom/pan, and table pagination via in-house event routing.
5. Add cross-platform visual + interaction regression tests; keep webview hosts as non-default reference comparators only.
