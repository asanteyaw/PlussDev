<!-- pluss-hand-authored -->

# flash::show

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
bool flash::show(
    Studio& studio, std::string_view title = {},
    std::int64_t width = 1280, std::int64_t height = 820, bool launch = true)
```

```{important} With launch = true it opens a native window
The launch = true path opens a window through `Studio::show`, as [`flash::animate3`](animate3.md) and [`flash::animate4`](animate4.md) do, so the examples that launch it are not run when the documentation is built. The function is a one-line forward (`return studio.show(title, width, height, launch);`) with no checks of its own; every argument is described on the page of [`Studio::show`](../studio/show.md).
```

## What It Does

A thin, familiar-naming wrapper around
[`Studio::show`](../studio/show.md) — forwards every argument unchanged, no
transformation or validation of its own.

## Parameters

**studio** (`Studio&`)
    The figure to display.

**title** (`std::string_view`, default empty)
    Forwarded to [`Studio::show`](../studio/show.md) as the window title.

**width**, **height** (`std::int64_t`, default `1280`, `820`)
    Forwarded to [`Studio::show`](../studio/show.md) as the window size.

**launch** (`bool`, default `true`)
    Forwarded to [`Studio::show`](../studio/show.md) — when `false`, the HTML is
    generated but no window is actually opened.

## Returns

`bool`
    Whatever [`Studio::show`](../studio/show.md) returns — see that page for the exact
    contract.

## Examples

Example 1. With `launch` false the figure is prepared but no window opens, and the call reports success.

```cpp
using namespace pluss::pl;

auto fig = studio("flash_show_nolaunch");
fig.pane2("curve").trend(x, y);
std::cout << "show(..., launch=false) returned: " << flash::show(fig, "My Figure", 1000, 700, false) << "\n";
```

Output:

```text
show(..., launch=false) returned: 1
```

Example 2. With the environment variable `PLUSSPL_CAPTURE_DIR` naming a directory, `show` records the figure there as `<figure name>.svg` instead of opening a window. This is how a program that calls `show` runs without a display.

```cpp
setenv("PLUSSPL_CAPTURE_DIR", dir.c_str(), 1);
auto fig = studio("flash_show_captured");
fig.pane2("curve").trend(x, y);
const bool shown = flash::show(fig);
unsetenv("PLUSSPL_CAPTURE_DIR");
std::cout << "show returned " << shown << "; captured file exists: " << fs::exists(dir / "flash_show_captured.svg") << "\n";
```

Output:

```text
show returned 1; captured file exists: 1
```

Example 3. The free function and `Studio::show` are the same operation.

```cpp
auto fig = studio("flash_show_equivalence");
fig.pane2("curve").trend(x, y);
std::cout << "flash::show and Studio::show agree when not launching: " << (flash::show(fig, {}, 1280, 820, false) == fig.show({}, 1280, 820, false)) << "\n";
```

Output:

```text
flash::show and Studio::show agree when not launching: 1
```

## Notes

- Not `noexcept`.
- No `flash::`-specific behavior exists beyond argument forwarding — this page exists
  for discoverability under the `flash::` namespace, not because the function does
  anything `Studio::show` doesn't already do.

## See Also

- [`Studio::show`](../studio/show.md): the underlying method this function forwards to
  directly, and where every argument's behavior is documented.
- [`flash::save_png`](save_png.md): the sibling export wrapper that writes to disk
  instead of opening a window.
- [`flash::animate3`](animate3.md), [`flash::animate4`](animate4.md): other
  window-launching functions in this reference sharing the same
  never-actually-invoked-with-launch=true verification boundary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="horizontal_grid.html">
    <span class="label">Previous</span>
    <span class="title">horizontal_grid</span>
  </a>
  <a class="next-link" href="save_png.html">
    <span class="label">Next</span>
    <span class="title">save_png</span>
  </a>
</div>
