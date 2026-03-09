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

  HyperSimulationOptions options;
  options.title = "Geometric 4D simulation";
  options.sweep_plane = HyperSimSweepPlane::XW;
  options.sweep_speed_deg_per_sec = 22.0;
  options.slice_default = 0.82;
  options.width = 1280;
  options.height = 860;

  const bool opened = simulate4(x4, y4, z4, w4, options);
  return opened ? 0 : 1;
}
