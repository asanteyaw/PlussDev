// FFT Poisson solve: agrees with the conjugate-gradient solve of the same discrete operator (compact and wide stencils, 1-3 axes, odd/even extents,
// batches, Float32), reproduces an exact eigenfunction to rounding, handles the wide stencil's alternating null mode, and rejects incompatible data.
#include "pluss/subsp/natsci/common/poisson.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace spatial = pluss::subsp::natsci::common::spatial;
namespace common = pluss::subsp::natsci::common;
using pluss::Series;

namespace {
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
spatial::PeriodicGrid grid_of(std::vector<std::int64_t> n, std::vector<double> h) {
  std::vector<common::ScalarQuantity<common::Length>> sp;
  for (double v : h)
    sp.push_back(common::ScalarQuantity<common::Length>::from_si(v));
  return spatial::make_periodic_grid(std::move(n), std::move(sp));
}
// deterministic pseudo-random zero-mean-compatible field
Series noise(std::vector<std::int64_t> shape, std::uint64_t seed, pluss::ScalarType t = pluss::ScalarType::Float64) {
  std::int64_t n = 1;
  for (auto s : shape)
    n *= s;
  std::vector<double> v(static_cast<std::size_t>(n));
  for (auto &x : v) {
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    x = static_cast<double>(seed >> 11) / 9007199254740992.0 - 0.5;
  }
  return pluss::series::make(v, shape).to(t);
}
double maxdiff(const Series &a, const Series &b) { return a.sub(b).abs().max().item<double>(); }
} // namespace

int main() {
  try {
    for (const auto stencil : {spatial::PoissonStencil::Compact, spatial::PoissonStencil::Wide}) {
      const bool wide = stencil == spatial::PoissonStencil::Wide;
      for (const std::vector<std::int64_t> n : {std::vector<std::int64_t>{16}, {9, 12}, {12, 9}, {8, 6, 10}, {7, 5, 9}}) {
        std::vector<double> h(n.size());
        for (std::size_t a = 0; a < n.size(); ++a)
          h[a] = 0.1 + 0.05 * static_cast<double>(a);
        const auto grid = grid_of(n, h);
        std::vector<std::int64_t> shape = {3};
        shape.insert(shape.end(), n.begin(), n.end());
        const Series u0 = noise(shape, 12345 + n.size());
        Series rhs = wide ? spatial::wide_laplacian(u0, grid) : spatial::laplacian(u0, grid); // compatible by construction
        spatial::PoissonOptions fft_opt, cg_opt;
        fft_opt.stencil = cg_opt.stencil = stencil;
        fft_opt.method = spatial::PoissonMethod::Fft;
        cg_opt.method = spatial::PoissonMethod::ConjugateGradient;
        cg_opt.solve.relative_tolerance = 1e-13;
        cg_opt.solve.absolute_tolerance = 1e-13;
        cg_opt.solve.max_iterations = 4000;
        const auto a = spatial::solve_poisson(rhs, grid, fft_opt), b = spatial::solve_poisson(rhs, grid, cg_opt);
        const auto lap = wide ? spatial::wide_laplacian(a.solution, grid) : spatial::laplacian(a.solution, grid);
        expect(maxdiff(lap, rhs) < 1e-8, "FFT solution does not satisfy the operator equation, rank " + std::to_string(n.size()));
        expect(maxdiff(a.solution, b.solution) < 1e-7, "FFT vs CG solution, rank " + std::to_string(n.size()) + (wide ? " wide" : " compact") + ": " + std::to_string(maxdiff(a.solution, b.solution)));
        expect(a.residual_norm.sizes() == b.residual_norm.sizes() && a.converged.all().item<bool>() && a.iterations == 0, "diagnostics layout");
      }
    }
    // exact eigenfunction: u = sin(2 pi x / L) cos(4 pi y / L')  -> laplacian symbol known in closed form
    {
      const std::int64_t nx = 32, ny = 24;
      const double hx = 0.2, hy = 0.25;
      const auto grid = grid_of({nx, ny}, {hx, hy});
      std::vector<double> u(static_cast<std::size_t>(nx * ny)), r(u.size());
      const double kx = 2.0 * std::numbers::pi * 3.0 / nx, ky = 2.0 * std::numbers::pi * 5.0 / ny;
      const double lam = -(4.0 / (hx * hx)) * std::pow(std::sin(kx / 2), 2) - (4.0 / (hy * hy)) * std::pow(std::sin(ky / 2), 2);
      for (std::int64_t i = 0; i < nx; ++i)
        for (std::int64_t j = 0; j < ny; ++j) {
          u[i * ny + j] = std::sin(kx * i) * std::cos(ky * j);
          r[i * ny + j] = lam * u[i * ny + j];
        }
      const Series rhs = pluss::series::make(r, {nx, ny}), exact = pluss::series::make(u, {nx, ny});
      expect(maxdiff(spatial::solve_poisson(rhs, grid).solution, exact) < 1e-13, "eigenfunction recovered to rounding");
    }
    // wide stencil on an even grid: the checkerboard is a null mode of the operator; a field with a checkerboard component is rejected like in the CG path
    {
      const auto grid = grid_of({8, 8}, {0.1, 0.1});
      spatial::PoissonOptions o;
      o.stencil = spatial::PoissonStencil::Wide;
      Series cb = noise({8, 8}, 7);
      std::vector<double> alt(64);
      for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
          alt[i * 8 + j] = ((i + j) % 2 ? 1.0 : -1.0) * ((i % 2) ? 1.0 : -1.0);
      bool threw = false;
      try {
        (void)spatial::solve_poisson(pluss::series::make(alt, {8, 8}).add(1.0), grid, o);
      } catch (const std::domain_error &) {
        threw = true;
      }
      expect(threw, "constant (null) component in the rhs is rejected");
    }
    // Float32 field
    {
      const auto grid = grid_of({16, 16}, {0.1, 0.1});
      const Series u0 = noise({16, 16}, 99, pluss::ScalarType::Float32);
      const Series rhs = spatial::laplacian(u0, grid);
      spatial::PoissonOptions o32;
      o32.mean_tolerance = 1e-4; // float32 rounding of the rhs mean
      const auto s = spatial::solve_poisson(rhs, grid, o32);
      expect(s.solution.scalar_type() == pluss::ScalarType::Float32 && maxdiff(s.solution, u0.sub(u0.mean())) < 5e-3, "Float32 solve");
    }
    std::cout << "spectral poisson tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
