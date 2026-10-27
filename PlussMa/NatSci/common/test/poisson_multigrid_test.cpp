// Poisson solves through multigrid-preconditioned CG against the plain CG baseline (bounded Dirichlet / Neumann / mixed ends with inhomogeneous data,
// all-flux gauge, 1-3 axes, batches) and against the exact spectral inverse on periodic grids; iteration counts must be mesh independent.
#include "pluss/subsp/natsci/common/poisson.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace spatial = pluss::subsp::natsci::common::spatial;
namespace common = pluss::subsp::natsci::common;
using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using pluss::Series;

namespace {
constexpr auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series noise(std::vector<std::int64_t> shape, std::uint64_t seed) {
  std::int64_t n = 1;
  for (auto s : shape)
    n *= s;
  std::vector<double> v(static_cast<std::size_t>(n));
  for (auto &x : v) {
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    x = static_cast<double>(seed >> 11) / 9007199254740992.0 - 0.5;
  }
  return pluss::series::make(v, shape).to(F64);
}
double maxdiff(const Series &a, const Series &b) { return a.sub(b).abs().max().item<double>(); }
spatial::BoxGrid box(std::vector<std::int64_t> n, double h) {
  std::vector<common::ScalarQuantity<common::Length>> sp(n.size(), common::ScalarQuantity<common::Length>::from_si(h));
  return spatial::make_box_grid(std::move(n), std::move(sp));
}
using Q = common::ScalarQuantity<Field>;
using QF = common::ScalarQuantity<Flux>;
spatial::Bounds<Field> bounds_for(const spatial::BoxGrid &g, const std::vector<int> &kind /* 0 Dirichlet, 1 Neumann per axis (both ends) */, double dval, double nval) {
  std::vector<spatial::Bound<Field>> rules;
  for (std::int64_t a = 0; a < g.dim(); ++a)
    for (auto side : {spatial::Side::Low, spatial::Side::High}) {
      const spatial::Face f{a, side};
      rules.push_back(kind[a] == 0 ? spatial::value_bc(f, Q::from_si(dval + 0.1 * a)) : spatial::flux_bc(f, QF::from_si(nval)));
    }
  return spatial::make_bounds<Field>(g, std::move(rules));
}
} // namespace

int main() {
  try {
    // ---- bounded: multigrid vs plain CG
    for (const auto &[n, kinds] : std::vector<std::pair<std::vector<std::int64_t>, std::vector<int>>>{
             {{48}, {0}}, {{32, 24}, {0, 1}}, {{24, 32}, {0, 0}}, {{16, 8, 12}, {1, 0, 0}}}) {
      const auto g = box(n, 0.1);
      const auto b = bounds_for(g, kinds, 0.7, 0.0);
      std::vector<std::int64_t> shape = {2};
      shape.insert(shape.end(), n.begin(), n.end());
      const Series rhs = noise(shape, 3 + n.size());
      spatial::PoissonOptions mgo, cgo;
      mgo.method = spatial::PoissonMethod::Multigrid;
      cgo.method = spatial::PoissonMethod::ConjugateGradient;
      mgo.solve.relative_tolerance = cgo.solve.relative_tolerance = 1e-12;
      mgo.solve.absolute_tolerance = cgo.solve.absolute_tolerance = 1e-12;
      cgo.solve.max_iterations = 5000;
      const auto a = spatial::solve_poisson(rhs, g, b, mgo), c = spatial::solve_poisson(rhs, g, b, cgo);
      expect(maxdiff(a.solution, c.solution) < 1e-7, "bounded multigrid vs CG, rank " + std::to_string(n.size()) + ": " + std::to_string(maxdiff(a.solution, c.solution)));
      expect(a.iterations <= 16 && a.iterations < c.iterations, "bounded multigrid iterations " + std::to_string(a.iterations) + " vs " + std::to_string(c.iterations));
    }
    // ---- all-flux gauge (singular) with a compatible rhs
    {
      const auto g = box({32, 32}, 0.1);
      const auto b = bounds_for(g, {1, 1}, 0.0, 0.0);
      Series rhs = noise({32, 32}, 5);
      rhs = rhs.sub(rhs.mean());
      spatial::PoissonOptions mgo, cgo;
      mgo.method = spatial::PoissonMethod::Multigrid;
      cgo.method = spatial::PoissonMethod::ConjugateGradient;
      cgo.solve.max_iterations = 4000;
      cgo.solve.relative_tolerance = 1e-12;
      mgo.solve.relative_tolerance = 1e-12;
      expect(maxdiff(spatial::solve_poisson(rhs, g, b, mgo).solution, spatial::solve_poisson(rhs, g, b, cgo).solution) < 1e-7, "all-flux multigrid vs CG");
    }
    // ---- mesh independence
    {
      std::int64_t prev = 0;
      for (std::int64_t n : {32, 64, 128, 256}) {
        const auto g = box({n, n}, 1.0 / static_cast<double>(n));
        const auto b = bounds_for(g, {0, 1}, 0.0, 0.0);
        spatial::PoissonOptions o;
        o.method = spatial::PoissonMethod::Multigrid;
        o.solve.relative_tolerance = 1e-10;
        o.solve.check_interval = 1;
        const auto r = spatial::solve_poisson(noise({n, n}, 9), g, b, o);
        expect(prev == 0 || r.iterations <= prev + 1, "box iterations grow with n");
        prev = r.iterations;
      }
      std::cout << "  box iterations at n=256: " << prev << "\n";
    }
    // ---- periodic: multigrid vs exact spectral
    {
      const auto g = spatial::make_periodic_grid({64, 48}, {common::ScalarQuantity<common::Length>::from_si(0.1), common::ScalarQuantity<common::Length>::from_si(0.1)});
      Series rhs = noise({2, 64, 48}, 4);
      rhs = rhs.sub(rhs.mean(std::vector<std::int64_t>{1, 2}, true));
      spatial::PoissonOptions mgo, ffo;
      mgo.method = spatial::PoissonMethod::Multigrid;
      mgo.solve.relative_tolerance = 1e-12;
      ffo.method = spatial::PoissonMethod::Fft;
      expect(maxdiff(spatial::solve_poisson(rhs, g, mgo).solution, spatial::solve_poisson(rhs, g, ffo).solution) < 1e-8, "periodic multigrid vs FFT");
      bool threw = false;
      try {
        mgo.stencil = spatial::PoissonStencil::Wide;
        (void)spatial::solve_poisson(rhs, g, mgo);
      } catch (const std::invalid_argument &) {
        threw = true;
      }
      expect(threw, "wide stencil + multigrid is rejected");
    }
    std::cout << "poisson multigrid tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
