#include <pluss/pluss.h>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

} // namespace

int main() {
  using namespace pluss::pl::flash;

  constexpr std::int64_t n = 720;
  const auto t = pluss::series::linspace(0.0, 1.0, n, pluss::ScalarType::Float64);

  // Stock-path style trajectory in 3D (time, price, volatility-like axis).
  const auto price =
      t.mul(230.0).add(t.mul(18.0 * kPi).sin().mul(26.0)).add(t.mul(40.0 * kPi).sin().mul(8.0)).add(105.0);
  const auto vol = t.mul(14.0 * kPi).sin().abs().mul(0.36).add(0.12);
  Animation3Options stock_opts;
  stock_opts.title = "3D stock-path animation";
  stock_opts.sweep_plane = Anim3SweepPlane::YZ;
  stock_opts.progressive_reveal = true;
  stock_opts.reveal_points_per_sec = 130.0;
  stock_opts.connect_path = true;
  stock_opts.snapshot_prefix = "plusspl_3d_stock";
  if (!animate3(t, price, vol, stock_opts)) {
    return 1;
  }

  // Engineering-style damped oscillation trajectory.
  const auto tau = pluss::series::linspace(0.0, 22.0, n, pluss::ScalarType::Float64);
  const auto envelope = tau.mul(-0.085).exp();
  const auto x = tau.mul(3.2).cos().mul(envelope);
  const auto y = tau.mul(3.2).sin().mul(envelope);
  const auto z = tau.mul(0.12);

  Animation3Options eng_opts;
  eng_opts.title = "3D engineering trajectory animation";
  eng_opts.sweep_plane = Anim3SweepPlane::XZ;
  eng_opts.progressive_reveal = true;
  eng_opts.reveal_points_per_sec = 120.0;
  eng_opts.connect_path = true;
  eng_opts.snapshot_prefix = "plusspl_3d_engineering";
  return animate3(x, y, z, eng_opts) ? 0 : 1;
}
