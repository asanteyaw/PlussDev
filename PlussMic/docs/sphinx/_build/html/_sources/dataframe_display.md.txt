# DataFrame Display And Popup Viewer

## Overview

PlussMic supports two display paths for table-style inspection:

- terminal rendering
- popup rendering through `.show()`

The popup viewer is opt-in.
Nothing opens automatically unless the user explicitly calls `.show()`.

## Which Methods Support `.show()`

`.show()` is available on methods that return `pluss::table::DataFrameView`.
Those are the table-rendering methods.

Current examples include:

- `head()`
- `tail()`
- `view()`
- `describe()`
- `transpose()`
- `aggregate()`
- `group()`
- `pivot()`
- `pivot_table()`
- `tally()`
- `resample()`
- `sum()`
- `mean()`
- `min()`
- `max()`
- `std()`
- `var()`
- `count()`

Examples:

```cpp
df.head();               // terminal preview
df.head().show();        // popup if available, else terminal fallback

df.describe();           // terminal preview
df.describe().show();    // popup if available, else terminal fallback

df.group().by({"city"}).agg({{"avg_sales", "sales", pluss::table::DataFrame::GroupOp::Mean}})
  .show();
```

If a method returns a plain `DataFrame` instead of `DataFrameView`, then it does not participate in this deferred popup-preview flow.

## How The Display Policy Works

The behavior is:

```cpp
df.head();          // prints in the terminal
df.head().show();   // suppresses terminal auto-preview and requests popup instead
```

This works because `head()`, `describe()`, and the other preview verbs return a lightweight `DataFrameView`.
That wrapper delays the terminal render until the expression ends.
If `.show()` is called first, the pending terminal render is consumed and the popup path is used instead.

## What `.show()` Takes

The primary overload is:

```cpp
view.show();
```

There is also an overload that accepts `DisplayOptions`:

```cpp
pluss::table::display::DisplayOptions opts;
view.show(opts);
```

There is also a stream-oriented overload:

```cpp
view.show(std::cout, opts);
```

In practice, the appropriate argument for `.show()` is `DisplayOptions`.

## Useful `DisplayOptions`

The most relevant display options for popup/table preview are:

- `backend`
- `viewer_width`
- `viewer_height`
- `window_title`
- `html_sortable`
- `html_search`
- `html_copy_button`

Example:

```cpp
pluss::table::display::DisplayOptions opts;
opts.viewer_width = 1440;
opts.viewer_height = 900;
opts.html_search = true;
opts.html_sortable = true;

df.describe().show(opts);
```

Notes:

- `backend = DisplayBackend::Terminal` forces terminal rendering
- the default `Auto` policy prefers the built-in `PlussWebView` renderer
- `backend = DisplayBackend::NativeWebView` explicitly requests the platform web view
- `backend = DisplayBackend::PlussWebView` explicitly requires the built-in renderer
- `window_title` is available, but for most users the better pattern is to name the frame with `title(...)` or `title_(...)`

Explicit native-renderer example:

```cpp
pluss::table::display::DisplayOptions opts;
opts.backend = pluss::table::display::DisplayBackend::NativeWebView;
df.show(opts);
```

## DataFrame Titles

Frames can carry display metadata:

```cpp
auto prices = df.title("prices");
prices.head().show();
prices.describe().show();
```

Or mutate in place:

```cpp
df.title_("prices");
df.head().show();
```

Behavior:

- untitled frame: `DataFrame - head`
- untitled frame: `DataFrame - describe`
- titled frame: `prices - head`
- titled frame: `prices - describe`

The title is replaceable metadata.

```cpp
df.title_("some title");
df.title_("some other title");
```

Immutable behavior:

```cpp
auto prices = df.title("prices");   // df unchanged
auto other = prices.title("derived"); // prices unchanged
```

## OS Support

Current popup support is:

- macOS: built-in PlussWebView and native WebKit
- Linux: built-in PlussWebView through X11/XWayland, with native WebKitGTK optional
- Windows: built-in PlussWebView through Win32, with native WebView2 optional

The default `Auto` policy tries PlussWebView first, then the available native
platform backend. If neither popup backend is available, `.show()` falls back
to the normal terminal rendering path.

## macOS Backend

On macOS, the popup viewer uses native Apple frameworks:

- `Cocoa`
- `WebKit`
- `QuartzCore`
- `UniformTypeIdentifiers`

That means the current macOS popup path does not depend on Conda GUI packages.
It uses OS-provided frameworks.

Practical consequence:

- a Conda environment is fine for the compiler/toolchain
- the popup viewer itself is not coming from Conda on macOS

## Linux Backend

The default Linux popup uses the built-in renderer in a direct X11 or
XWayland window. It does not require GTK or WebKitGTK. FreeType is used for
native-quality text when it is available, and XInput2 provides high-resolution
touchpad and mouse-wheel input when it is available.

Typical packages for building the staged SDK, or linking a downstream
application to the current static SDK, are:

Ubuntu:

```bash
sudo apt install libx11-dev libxi-dev libfreetype-dev
```

Fedora:

```bash
sudo dnf install libX11-devel libXi-devel freetype-devel
```

The staged application needs an X11 server or XWayland at runtime. A
Wayland-only session with XWayland disabled cannot currently open the built-in
window; `Auto` then tries the optional native backend before falling back to
terminal output.

GTK and WebKitGTK are needed only when building and explicitly selecting
`DisplayBackend::NativeWebView`.

## Windows Status

The default Windows popup uses the built-in renderer in a native Win32 window.
It uses Windows system APIs for windowing, input, clipboard, saving, and text,
so neither the WebView2 SDK nor the WebView2 Runtime is required for `.show()`.

## Windows Build Setup

No additional viewer SDK is required for the built-in Windows renderer. A
normal staged Pluss SDK built for the target architecture includes it.

WebView2 remains an optional native web-view backend. To build it, Pluss looks
for:

- cache variable: `PLUSSTILS_VIEW_WEBVIEW2_SDK_DIR`
- environment variable: `WEBVIEW2_SDK_DIR`
- common NuGet cache location under the user profile

Typical configure command:

```bash
cmake -S . -B build ^
  -DPLUSSTILS_VIEW_WEBVIEW2_SDK_DIR="C:/path/to/Microsoft.Web.WebView2.<version>"
```

Disabling WebView2 does not disable the built-in popup:

```bash
cmake -S . -B build -DPLUSSTILS_VIEW_ENABLE_WINDOWS_WEBVIEW2=OFF
```

What the SDK directory should contain:

- `build/native/include/WebView2.h`
- `build/native/x64/WebView2LoaderStatic.lib`
  or the equivalent `x86` / `arm64` loader library for the target architecture

## Conda On Windows

Conda can provide the compiler and normal dependencies. The default popup
itself comes from the built-in renderer and Windows system APIs. WebView2 is
relevant only when the optional native backend is requested.

## Are The Popup Windows Native

Partly.

The host window is native to the selected presentation backend:

- built-in PlussWebView on macOS: Cocoa window with software-rendered content
- built-in PlussWebView on Linux: X11/XWayland window with software-rendered content
- built-in PlussWebView on Windows: Win32 window with software-rendered content
- native macOS backend: Cocoa window with embedded WebKit
- native Linux backend: GTK window with embedded WebKitGTK
- native Windows backend: Win32 window with embedded WebView2

PlussWebView parses and renders the supported HTML/CSS itself. Native backends
render the same document inside the platform web view.

So the actual behavior is:

- window chrome feels native to the OS/toolkit
- table content stays visually consistent across platforms because it is HTML-driven

## Conda Notes

Conda support differs by platform.

- macOS:
  no GTK/WebKitGTK package is needed for the current implementation
- Linux:
  built-in PlussWebView needs the standard X11 runtime (or XWayland); FreeType
  is used when available for native-quality fonts. GTK/WebKitGTK is needed only
  when explicitly selecting the native web-view backend
- Windows:
  built-in PlussWebView uses Windows system APIs and does not require WebView2.
  The WebView2 SDK and Runtime are needed only for the optional native backend

## Recommended User Guidance

The simplest explanation for users is:

```cpp
df.head();          // terminal
df.head().show();   // popup if supported, else terminal
```

If they want better popup titles:

```cpp
auto prices = df.title("prices");
prices.head().show();      // prices - head
prices.describe().show();  // prices - describe
```

## Demo

See:

- `PlussMic/examples/table/display_policy_demo.cpp`

That demo shows:

- terminal preview
- popup preview with `.show()`
- explicit frame titles with `title("prices")`

## External Setup References

Official references that are useful when setting up popup support:

- WebView2 overview and installation:
  https://learn.microsoft.com/en-us/microsoft-edge/webview2/
- WebView2 Runtime distribution:
  https://developer.microsoft.com/en-us/microsoft-edge/webview2/
- WebView2 SDK package:
  https://www.nuget.org/packages/Microsoft.Web.WebView2
- Conda GTK3 package:
  https://anaconda.org/conda-forge/gtk3
- Conda WebKitGTK package:
  https://anaconda.org/conda-forge/webkit2gtk4.1
- Ubuntu `libwebkit2gtk-4.1-dev`:
  https://packages.ubuntu.com/noble/libwebkit2gtk-4.1-dev
- Ubuntu `libgtk-3-dev`:
  https://packages.ubuntu.com/noble/libgtk-3-dev
- Fedora `webkit2gtk4.1-devel`:
  https://packages.fedoraproject.org/pkgs/webkitgtk/webkit2gtk4.1-devel
- Fedora `gtk3-devel`:
  https://packages.fedoraproject.org/pkgs/gtk3/gtk3-devel
