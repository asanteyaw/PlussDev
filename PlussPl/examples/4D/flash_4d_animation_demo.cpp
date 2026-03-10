#include <pluss/pluss.h>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

int main() {
  using namespace pluss::pl::flash;

  constexpr std::int64_t count = 620;
  const auto t = pluss::linspace(0.0, 2.0 * kPi, count, pluss::ScalarType::Float64);
  const auto x4 = t.cos();
  const auto y4 = t.sin();
  const auto z4 = t.mul(2.0).cos().mul(0.72);
  const auto w4 = t.mul(3.0).sin().mul(0.58);

  HyperAnimationOptions options;
  options.title = "Geometric 4D simulation";
  options.sweep_plane = HyperAnimateSweepPlane::XW;
  options.sweep_speed_deg_per_sec = 22.0;
  options.slice_default = 0.82;
  options.width = 1280;
  options.height = 860;
  options.snapshot_prefix = "plusspl_flash_4d_live";

  const bool opened = animate4(x4, y4, z4, w4, options);
  if (!opened) {
    return 1;
  }

  HyperFrameExportOptions export_opts;
  export_opts.output_dir = "PlussPl/examples/outputs/4D/plusspl_flash_4d_frame_export";
  export_opts.file_prefix = "hyper4d";
  export_opts.title = "Geometric 4D frame";
  export_opts.width = 1280;
  export_opts.height = 860;
  export_opts.sweep.frames = 16;
  export_opts.sweep.plane = HyperSweepPlane::XW;
  export4_frames(x4, y4, z4, w4, export_opts);
  return 0;
}
