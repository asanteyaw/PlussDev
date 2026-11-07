// The mesh and the frame. The frame is the poll radius; the mesh is the finer grid the poll points lie on. After an
// unsuccessful iteration the frame is multiplied by `frame_refine_factor` (default 0.5) and the mesh by `mesh_refine_factor`
// (0.25); after a success they grow by `frame_expand_factor` (2) and `mesh_expand_factor` (4), and the mesh never exceeds
// the frame. `initial_frame_size` and `initial_mesh_size` (0 = chosen from the start point) set the starting scale, and
// the run converges when the frame falls below `mesh_tolerance` (the improvement-window test of `check_every` can end a run
// earlier, so the mesh_tolerance rows switch it off with check_every = 0). `opportunistic` stops a poll at the first improvement;
// off, the whole poll is evaluated and the best point taken. Same 4-variable quadratic for every row; its minimum value is 1.
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  std::vector<std::pair<const char*, dfo::MadsBlackboxConfig>> cases;
  dfo::MadsBlackboxConfig base;
  cases.emplace_back("defaults", base);
  auto big = base; big.initial_frame_size = 8.0; big.initial_mesh_size = 8.0;
  cases.emplace_back("initial frame and mesh 8", big);
  auto tiny = base; tiny.initial_frame_size = 0.01; tiny.initial_mesh_size = 0.01;
  cases.emplace_back("initial frame and mesh 0.01", tiny);
  auto gentle = base; gentle.frame_refine_factor = 0.8; gentle.mesh_refine_factor = 0.64;
  cases.emplace_back("gentle refinement (0.8, 0.64)", gentle);
  auto aggressive = base; aggressive.frame_refine_factor = 0.25; aggressive.mesh_refine_factor = 0.0625;
  cases.emplace_back("aggressive refinement", aggressive);
  auto tight = base; tight.check_every = 0; tight.mesh_tolerance = 1e-12;
  cases.emplace_back("mesh_tolerance 1e-12, no window", tight);
  auto mid = base; mid.check_every = 0; mid.mesh_tolerance = 1e-8;
  cases.emplace_back("mesh_tolerance 1e-8, no window", mid);
  auto loose = base; loose.check_every = 0; loose.mesh_tolerance = 1e-3;
  cases.emplace_back("mesh_tolerance 1e-3, no window", loose);
  auto full = base; full.opportunistic = false;
  cases.emplace_back("opportunistic off", full);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    auto f = [](const pluss::Series& x) {
      const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
      const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
      return d.square().sum() + s.square() * 0.8 + 1.0;
    };
    c.max_iterations = 4000;
    c.max_evaluations = 40000;
    const auto o = dfo::orthomads_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
