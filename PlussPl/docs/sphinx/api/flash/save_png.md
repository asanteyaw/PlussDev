<!-- pluss-hand-authored -->

# flash::save_png

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
void flash::save_png(Studio& studio, std::string_view path, std::int64_t dpi = 144)
```

## What It Does

A thin wrapper around [`Studio::save_png`](../studio/save_png.md) — forwards `path` and
`dpi` unchanged, no transformation or validation of its own. Unlike
[`flash::show`](show.md)/[`flash::animate3`](animate3.md)/[`flash::animate4`](animate4.md),
this function writes a file to disk and opens no window.

## Parameters

**studio** (`Studio&`)
    The figure to render.

**path** (`std::string_view`)
    Forwarded to [`Studio::save_png`](../studio/save_png.md) as the output file path.

**dpi** (`std::int64_t`, default `144`)
    Forwarded to [`Studio::save_png`](../studio/save_png.md) as the render resolution.

## Returns

`void`

## Examples

Example 1. A PNG written to disk; the program checks that the file exists afterwards.

```cpp
using namespace pluss::pl;
namespace fs = std::filesystem;

auto fig = studio("flash_save_png_demo");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1, 2, 3}, std::vector<double>{1, 4, 9});
const std::string out_path = "/tmp/flash_save_png_demo.png";
flash::save_png(fig, out_path, 96);
std::cout << "save_png wrote file exists: " << fs::exists(out_path) << ", non-empty: " << (fs::file_size(out_path) > 0) << "\n";
```

Output:

```text
save_png wrote file exists: 1, non-empty: 1
```

## Notes

- Not `noexcept`.
- No `flash::`-specific behavior exists beyond argument forwarding — see
  [`Studio::save_png`](../studio/save_png.md) for the full, independently-documented
  contract (including its own validation/error behavior, if any).

## See Also

- [`Studio::save_png`](../studio/save_png.md): the underlying method this function
  forwards to directly.
- [`flash::show`](show.md): the sibling wrapper that opens an interactive window
  instead of writing to disk.
- [`flash::export4_frames`](export4_frames.md): the 4D-animation-sequence export
  function, which internally calls `Studio::save_png` (among other formats) once per
  frame rather than through this wrapper.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="flash_palette.html">
    <span class="label">Next</span>
    <span class="title">flash_palette</span>
  </a>
</div>
