<!-- pluss-hand-authored -->

# flash::export4_frames

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
template <class X, class Y, class Z, class W>
std::int64_t flash::export4_frames(X&& x, Y&& y, Z&& z, W&& w,
                                    const HyperFrameExportOptions& options = {})
```

## What It Does

Renders a full 4D rotation sweep to a numbered sequence of static image files on
disk — the file-writing counterpart to [`flash::animate4`](animate4.md)'s live,
in-browser animation. Internally: calls
[`flash::hyper_animation_sequence`](hyper_animation_sequence.md) on `options.plot` and
`options.sweep` to get one `HyperPlotOptions` snapshot per frame, then for each
snapshot creates a fresh `Studio`, renders the projected `(x, y, z, w)` cloud into it
via [`flash::cloud4`](cloud4.md), and saves that frame to
`{output_dir}/{file_prefix}_{zero-padded index}.{ext}` using `Studio::save_png`,
`Studio::save_svg`, or `Studio::save_pdf` depending on `options.format`. Unlike
`animate4`, this function was fully, directly verified this session — it writes real
files and touches no window/GUI code at all.

## Parameters

**x**, **y**, **z**, **w** (series-convertible, must each be rank ≤ 3 and identical
lengths)
    The 4D point data, reused unchanged for every exported frame — only the rotation
    angles change frame to frame, per `options.sweep`. Unequal lengths throw
    `std::invalid_argument("flash::export4_frames: x, y, z, w must have identical
    lengths")`.

**options** (`const HyperFrameExportOptions&`, optional)
    See the field table below.

## `HyperFrameExportOptions` fields

| Field | Type and default | Description |
|---|---|---|
| `output_dir` | `std::filesystem::path`, default `"PlussPl/examples/outputs/4D/plusspl_flash_4d_frame_export"` | Directory the frame files are written into — created (including any missing parent directories) via `std::filesystem::create_directories` if it doesn't already exist. Must be non-empty, or `export4_frames` throws `std::invalid_argument("flash::export4_frames: output_dir must be non-empty")`. |
| `file_prefix` | `std::string`, default `"frame"` | Filename prefix for every exported frame. Frame index `i` is written as `{file_prefix}_{i, zero-padded to the width of the largest index}.{ext}` — e.g. with 3 frames (indices `0`–`2`, one digit each) and the default prefix, files are named `frame_0.png`, `frame_1.png`, `frame_2.png`; with 10+ frames the indices are zero-padded to two or more digits so the files still sort correctly by name. |
| `title` | `std::string`, default `"Geometric 4D frame"` | Forwarded as each frame's `Studio`/pane title (the same `title` argument [`flash::cloud4`](cloud4.md)'s `Studio` overload accepts) — identical across every exported frame. |
| `format` | `AnimationFrameFormat`, default `Png` | `Png` (via `Studio::save_png`, using `dpi`), `Svg` (via `Studio::save_svg`), or `Pdf` (via `Studio::save_pdf`) — also determines each file's extension (`.png`/`.svg`/ `.pdf`). |
| `width`, `height` | `std::int64_t`, default `1280`, `860` | The `Studio` figure size (in points, via `Studio::size`) each frame is rendered at. Both must be at least `480`×`320`, or `export4_frames` throws `std::invalid_argument("flash::export4_frames: invalid frame extent")`. |
| `dpi` | `std::int64_t`, default `144` | Resolution used only when `format = Png`, passed to `Studio::save_png` — clamped to at least `72` internally (`std::max<std::int64_t>(72, options.dpi)`), so a `dpi` below `72` does not throw but is silently raised to `72`. Unused for `Svg`/`Pdf`. |
| `theme` | `Theme`, default `Theme::paper()` | The theme each per-frame `Studio` is constructed with. |
| `plot` | `HyperPlotOptions`, default `{}` | The base 4D plot configuration — see [`flash::path4`](path4.md)'s field table. `plot.point_size` is the one field genuinely used by [`flash::cloud4`](cloud4.md)'s per-frame render; every field of `plot` except whichever one `sweep.plane` selects (see below) is held fixed across all exported frames. |
| `sweep` | `HyperSweepOptions`, default `{}` | Forwarded, together with `plot`, straight into [`flash::hyper_animation_sequence`](hyper_animation_sequence.md) — see that page's field table for `frames`/`plane`/`start_deg`/`end_deg`/`include_endpoint`. A `sweep.frames` of `1` or fewer propagates `hyper_animation_sequence`'s own `std::invalid_argument("flash::hyper_animation_sequence: frames must be > 1")` rather than a distinct `export4_frames`-specific message. |

## Returns

`std::int64_t`
    The number of frames actually written — always equal to `options.sweep.frames`
    (barring the empty-sequence guard below), confirmed directly by counting real files
    on disk after the call. Not a `bool`/success flag, unlike
    [`flash::animate3`](animate3.md)/[`flash::animate4`](animate4.md)'s return type.

## Examples

Example 1. Three frames, `SVG` format, sweeping the `XW` plane over a short range. The
embedded image below is the exact `frame_0.svg` written by this block.

```cpp
using namespace pluss::pl;

auto t = pluss::series::linspace(
    0.0,
    2.0 * 3.14159265358979323846,
    360,
    pluss::ScalarType::Float64);
auto hx = t.cos();
auto hy = t.sin();
auto hz = t.mul(2.0).cos().mul(0.72);
auto hw = t.mul(3.0).sin().mul(0.58);

flash::HyperFrameExportOptions opts;
opts.output_dir = "/tmp/plusspl_export4_frames_demo";
opts.file_prefix = "frame";
opts.format = flash::AnimationFrameFormat::Svg;
opts.width = 960;
opts.height = 620;
opts.sweep.frames = 3;
opts.sweep.plane = flash::HyperSweepPlane::XW;
opts.sweep.start_deg = 0.0;
opts.sweep.end_deg = 45.0;
opts.sweep.include_endpoint = true;
const auto count = flash::export4_frames(hx, hy, hz, hw, opts);
std::cout << "export4_frames wrote " << count << " frames\n";

std::size_t actual_files = 0;
for (const auto& entry : std::filesystem::directory_iterator(opts.output_dir.string())) {
  (void)entry;
  ++actual_files;
}
std::cout << "export4_frames actual files on disk: " << actual_files << "\n";
```

Output:

```text
export4_frames wrote 3 frames
export4_frames actual files on disk: 3
```

The three files created were `frame_0.svg`, `frame_1.svg`, `frame_2.svg`.

![Output of Example 1 (`frame_0.svg`)](../../_static/plots/methods/export4_frames.svg)

The image above is the exact `frame_0.svg` file written by the block above. The
`std::cout` lines only report how many files were written; they do not render the
picture.

Example 2. A later frame from a different sweep configuration. This demonstrates that
`export4_frames(...)` is not locked to a single plane or a single start/end range; the
embedded image below is the exact `frame_2.svg` produced by this block.

```cpp
using namespace pluss::pl;

auto t = pluss::series::linspace(
    0.0,
    2.0 * 3.14159265358979323846,
    360,
    pluss::ScalarType::Float64);
auto hx = t.cos();
auto hy = t.sin();
auto hz = t.mul(2.0).cos().mul(0.72);
auto hw = t.mul(3.0).sin().mul(0.58);

flash::HyperFrameExportOptions opts;
opts.output_dir = "/tmp/plusspl_export4_frames_demo_alt";
opts.file_prefix = "frame";
opts.format = flash::AnimationFrameFormat::Svg;
opts.width = 960;
opts.height = 620;
opts.sweep.frames = 4;
opts.sweep.plane = flash::HyperSweepPlane::YW;
opts.sweep.start_deg = -30.0;
opts.sweep.end_deg = 30.0;
opts.sweep.include_endpoint = true;
const auto count = flash::export4_frames(hx, hy, hz, hw, opts);
std::cout << "export4_frames alt wrote " << count << " frames\n";
```

Output:

```text
export4_frames alt wrote 4 frames
```

![Output of Example 2 (`frame_2.svg`)](../../_static/plots/methods/export4_frames_frame2.svg)

The image above is the exact exported `frame_2.svg` written by the block above, not the
console output.

Example 3 (edge case). `width` below the `480` minimum throws before any frame is
rendered or written.

```cpp
using namespace pluss::pl;

std::vector<double> x{1.0, 2.0};
std::vector<double> y{1.0, 2.0};
std::vector<double> z{1.0, 2.0};
std::vector<double> w{1.0, 2.0};
flash::HyperFrameExportOptions opts;
opts.width = 100;
try {
  flash::export4_frames(x, y, z, w, opts);
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << "export4_frames(width too small) threw: " << msg.substr(0, msg.find('\n')) << "\n";
}
```

Output:

```text
export4_frames(width too small) threw: flash::export4_frames: invalid frame extent
```

## Notes

- Not `noexcept`.
- Each frame is a fully independent `Studio`/pane render (a fresh `Studio` per frame,
  not one figure reused and re-saved) — there is no shared state between frames beyond
  the input data and `options`.
- Frame index zero-padding width is computed from the *largest* index
  (`frames.size() - 1`), so it adapts automatically to `sweep.frames`; no separate
  padding-width option exists.
- `output_dir` is created recursively if missing; this function never fails because a
  directory doesn't yet exist, only because `output_dir` itself is empty.
- Because it writes real files with no GUI/window step, this is the one member of the
  3D/4D scene-and-animation family in this task that was verified end-to-end by
  actually invoking it, not just by validating its pre-flight guards — contrast with
  [`flash::animate3`](animate3.md)/[`flash::animate4`](animate4.md).

## See Also

- [`flash::hyper_animation_sequence`](hyper_animation_sequence.md): what this function
  calls internally to compute each frame's rotation.
- [`flash::cloud4`](cloud4.md): what each individual frame is rendered through.
- [`flash::animate4`](animate4.md): the live, in-browser equivalent of sweeping through
  the same rotation range, without writing files.
- [`Studio::save_png`](../studio/save_png.md), [`Studio::save_svg`](../studio/save_svg.md),
  [`Studio::save_pdf`](../studio/save_pdf.md): the underlying per-frame file writers,
  selected by `options.format`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="animate4.html">
    <span class="label">Previous</span>
    <span class="title">animate4</span>
  </a>
  <a class="next-link" href="set_theme.html">
    <span class="label">Next</span>
    <span class="title">set_theme</span>
  </a>
</div>
