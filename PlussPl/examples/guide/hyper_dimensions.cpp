// Four dimensions on a flat screen: a trajectory and a cloud in (x, y, z, w), the rotations that reveal the fourth axis, and the frames of
// a turn written out as an animation. Opening a live viewer is left to an environment switch, since the guide runs without a window.
#include <pluss/pl/plusspl.h>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace pluss::pl;

static std::string out(const char *name) {
  const char *dir = std::getenv("PLUSSMA_GUIDE_ASSETS");
  return ((dir ? std::filesystem::path(dir) : std::filesystem::path(".")) / name).string();
}


int main() {
  // A rotating helix in four dimensions: two circles of different speed.
  const std::int64_t n = 400;
  auto t = pluss::series::linspace(0.0, 4.0 * 3.141592653589793, n, pluss::ScalarType::Float64);
  auto x = t.cos(), y = t.sin(), z = (t * 0.5).cos(), w = (t * 0.5).sin();
  std::cout << "path of " << n << " points in four dimensions\n";

  // 1. A 4D path and a 4D cloud: the options choose how w is shown (perspective distance, a slice, or an orthographic drop).
  auto fig = studio("hyper_dimensions");
  fig.mosaic(1, 2);
  fig.headline("A helix in four dimensions");
  auto &path = fig.pane3("path4");
  flash::path4(path, x, y, z, w);
  auto &cloud = fig.pane3("cloud4 with a rotation in the x-w plane");
  flash::cloud4(cloud, x, y, z, w, flash::HyperPlotOptions{.rot_xw_deg = 40.0, .point_size = 6.0});
  fig.show();

  // 2. hyper_animation_sequence gives the options of every frame of a full turn in one plane.
  const auto frames = flash::hyper_animation_sequence(flash::HyperPlotOptions{}, flash::HyperSweepOptions{.frames = 12, .plane = flash::HyperSweepPlane::XW});
  std::cout << "a turn in the x-w plane in " << frames.size() << " frames; angle of frame 3: " << frames[3].rot_xw_deg << " degrees\n";

  // 3. export4_frames writes the frames to a folder, ready for a video tool.
  const auto directory = std::filesystem::path(std::getenv("PLUSSMA_GUIDE_ASSETS") ? std::getenv("PLUSSMA_GUIDE_ASSETS") : ".") / "hyper_frames";
  std::filesystem::remove_all(directory);
  flash::HyperFrameExportOptions options;
  options.output_dir = directory;
  options.format = flash::AnimationFrameFormat::Svg;
  options.sweep.frames = 6;
  const auto written = flash::export4_frames(x, y, z, w, options);
  std::cout << "frames written: " << written << "\n";
  std::filesystem::remove_all(directory);

  // 4. A live viewer: animate3 spins a 3D plot and animate4 sweeps a 4D one. They open a window, so run them only on request.
  if (std::getenv("PLUSSPL_WINDOW")) {
    flash::animate3(x, y, z);
    flash::animate4(x, y, z, w);
  } else {
    std::cout << "live viewers skipped (set PLUSSPL_WINDOW=1 to open animate3 and animate4)\n";
  }
}
