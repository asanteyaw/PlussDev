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
- `group_by()`
- `pivot()`
- `pivot_table()`
- `count_by()`
- `mean_by()`
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

df.group_by({"city"}, {{"avg_sales", "sales", pluss::table::DataFrame::GroupOp::Mean}})
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
- `backend = DisplayBackend::NativeWebView` requests the GUI path when a native backend exists
- `window_title` is available, but for most users the better pattern is to name the frame with `title(...)` or `title_(...)`

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

- macOS: supported
- Linux: supported
- Windows: supported when WebView2 support is compiled in and the WebView2 Runtime is installed

If the popup backend is unavailable, `.show()` falls back to the normal terminal rendering path.

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

On Linux, the popup viewer uses:

- `gtk+-3.0`
- `webkit2gtk-4.1`
- fallback to `webkit2gtk-4.0`

Those are discovered through `pkg-config`.

Typical system packages:

Ubuntu:

```bash
sudo apt install pkg-config libgtk-3-dev libwebkit2gtk-4.1-dev
```

Fedora:

```bash
sudo dnf install pkgconf-pkg-config gtk3-devel webkit2gtk4.1-devel
```

Linux Conda environments can often provide the GTK/WebKit pieces, but the most reliable path is still the system packages plus `pkg-config`.

## Windows Status

On Windows, the popup viewer uses:

- native Win32 windowing
- Microsoft Edge WebView2 for the embedded HTML table

That means the host window is native Windows UI, and the table itself is still rendered from the same HTML/CSS/JavaScript payload used on the other platforms.

There are two separate Windows requirements:

- build-time requirement: the WebView2 SDK headers and loader library must be available when compiling PlussMic from source
- runtime requirement: the Microsoft Edge WebView2 Runtime must be installed on the target machine

If either one is missing, `.show()` falls back to terminal rendering.

## Windows Build Setup

The Windows viewer backend is enabled when CMake can find the WebView2 SDK.

PlussMic looks for:

- cache variable: `PLUSSMIC_WEBVIEW2_SDK_DIR`
- environment variable: `WEBVIEW2_SDK_DIR`
- common NuGet cache location under the user profile

Typical configure command:

```bash
cmake -S . -B build ^
  -DPLUSSMIC_WEBVIEW2_SDK_DIR="C:/path/to/Microsoft.Web.WebView2.<version>"
```

If you want to disable Windows popup support explicitly:

```bash
cmake -S . -B build -DPLUSSMIC_ENABLE_WINDOWS_WEBVIEW2=OFF
```

What the SDK directory should contain:

- `build/native/include/WebView2.h`
- `build/native/x64/WebView2LoaderStatic.lib`
  or the equivalent `x86` / `arm64` loader library for the target architecture

## Windows Runtime Setup

Even if PlussMic builds with WebView2 support, the target machine still needs the WebView2 Runtime.

That runtime is often already present on modern Windows systems, but it should still be treated as a requirement for popup viewing.

If the runtime is missing:

- `.show()` does not crash
- it simply falls back to terminal rendering

## Conda On Windows

Conda is not the source of the Windows popup backend.

On Windows:

- your Conda environment can still provide the compiler toolchain and normal build environment
- the popup viewer itself comes from Windows + WebView2
- users should not expect a Conda package to supply the full native popup stack

So the practical rule is:

- Conda can help with building
- WebView2 SDK and Runtime are the Windows viewer requirements

## Are The Popup Windows Native

Partly.

The host window is native to the platform backend:

- macOS: native Cocoa window
- Linux: GTK window
- Windows: native Win32 window with embedded WebView2

The table content itself is rendered as HTML/CSS/JavaScript inside the embedded web view.

So the actual behavior is:

- window chrome feels native to the OS/toolkit
- table content stays visually consistent across platforms because it is HTML-driven

## Conda Notes

Conda support differs by platform.

- macOS:
  no GTK/WebKitGTK package is needed for the current implementation
- Linux:
  Conda may provide the needed GTK/WebKit packages, but system packages are usually simpler and more reliable
- Windows:
  Conda does not provide the native viewer stack; use WebView2 SDK + Runtime

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
