// Geometric multigrid: operator vs an explicit scalar-loop construction, transfer adjointness, symmetry, solves vs plain CG, mesh-independent iteration
// counts, variable (jumping) coefficients, singular problems, batch lanes.
#include "pluss/subsp/math/multigrid.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
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
double dot(const Series &a, const Series &b) { return a.mul(b).sum().item<double>(); }
double maxdiff(const Series &a, const Series &b) { return a.sub(b).abs().max().item<double>(); }
std::vector<std::int64_t> shape_of(const std::vector<mg::Axis> &axes, std::int64_t batch = 0) {
  std::vector<std::int64_t> s;
  if (batch)
    s.push_back(batch);
  for (const auto &a : axes)
    s.push_back(a.cells);
  return s;
}
double ghost_sign(mg::Edge e) { return e == mg::Edge::Dirichlet ? -1.0 : 1.0; }

// explicit loop oracle (2-D, no batch): (A u)(i,j) = -sum_a h^-2 [ b+ (u+ - u) - b- (u - u-) ]
std::vector<double> loop_apply(const std::vector<mg::Axis> &ax, const std::vector<double> &u, const std::vector<std::vector<double>> &bp, const std::vector<std::vector<double>> &bm) {
  const std::int64_t n0 = ax[0].cells, n1 = ax[1].cells;
  auto at = [&](std::int64_t i, std::int64_t j) { return u[static_cast<std::size_t>(i * n1 + j)]; };
  std::vector<double> out(u.size());
  for (std::int64_t i = 0; i < n0; ++i)
    for (std::int64_t j = 0; j < n1; ++j) {
      double acc = 0.0;
      for (int a = 0; a < 2; ++a) {
        const std::int64_t n = ax[a].cells;
        const std::int64_t k = a == 0 ? i : j;
        auto val = [&](std::int64_t kk) { return a == 0 ? at(kk, j) : at(i, kk); };
        const double c = at(i, j);
        double up, dn;
        if (k + 1 < n) up = val(k + 1); else up = ax[a].high == mg::Edge::Periodic ? val(0) : ghost_sign(ax[a].high) * c;
        if (k > 0) dn = val(k - 1); else dn = ax[a].low == mg::Edge::Periodic ? val(n - 1) : ghost_sign(ax[a].low) * c;
        const std::size_t idx = static_cast<std::size_t>(i * n1 + j);
        acc += (bp[a][idx] * (up - c) - bm[a][idx] * (c - dn)) / (ax[a].spacing * ax[a].spacing);
      }
      out[static_cast<std::size_t>(i * n1 + j)] = -acc;
    }
  return out;
}
} // namespace

int main() {
  try {
    // ---- operator definition vs scalar loops, all edge combinations
    {
      for (auto lo0 : {mg::Edge::Periodic, mg::Edge::Dirichlet, mg::Edge::Neumann})
        for (auto lo1 : {mg::Edge::Periodic, mg::Edge::Dirichlet, mg::Edge::Neumann}) {
          std::vector<mg::Axis> ax = {{8, 0.3, lo0, lo0}, {6, 0.45, lo1, lo1}};
          if (lo0 == mg::Edge::Dirichlet) ax[0].high = mg::Edge::Neumann; // mixed ends
          const Series bp0 = noise({8, 6}, 1).add(0.75), bp1 = noise({8, 6}, 2).add(0.75), bm0 = noise({8, 6}, 3).add(0.75), bm1 = noise({8, 6}, 4).add(0.75);
          mg::Problem p{.axes = ax, .face_low = {bm0, bm1}, .face_high = {bp0, bp1}};
          const Series like = noise({8, 6}, 9);
          mg::Hierarchy h(p, like);
          const Series u = noise({8, 6}, 10);
          const auto expected = loop_apply(ax, u.host<double>().array().data, {bp0.host<double>().array().data, bp1.host<double>().array().data}, {bm0.host<double>().array().data, bm1.host<double>().array().data});
          expect(maxdiff(h.apply(u), pluss::series::make(expected, {8, 6}).to(F64)) < 1e-12, "operator vs loops");
        }
    }
    // ---- symmetry and positive semi-definiteness of the V-cycle preconditioner (and the operator), 1-3 axes with mixed ends
    for (const auto &axes : {std::vector<mg::Axis>{{256, 0.1, mg::Edge::Dirichlet, mg::Edge::Neumann}},
                             std::vector<mg::Axis>{{16, 0.1, mg::Edge::Periodic, mg::Edge::Periodic}, {16, 0.1, mg::Edge::Dirichlet, mg::Edge::Dirichlet}},
                             std::vector<mg::Axis>{{8, 0.1, mg::Edge::Neumann, mg::Edge::Neumann}, {16, 0.1, mg::Edge::Periodic, mg::Edge::Periodic}, {8, 0.1, mg::Edge::Dirichlet, mg::Edge::Neumann}}}) {
      std::vector<Series> bl, bh;
      for (std::size_t a = 0; a < axes.size(); ++a) {
        bl.push_back(noise(shape_of(axes), 20 + a).abs().add(0.5));
        bh.push_back(noise(shape_of(axes), 30 + a).abs().add(0.5));
      }
      (void)bl;
      mg::Hierarchy h({.axes = axes, .face_high = bh}, noise(shape_of(axes), 1));
      const Series x = noise(shape_of(axes), 41), y = noise(shape_of(axes), 42);
      expect(std::abs(dot(h.apply(x), y) - dot(x, h.apply(y))) < 1e-10 * (1.0 + std::abs(dot(h.apply(x), y))), "operator symmetry");
      expect(std::abs(dot(h.vcycle(x), y) - dot(x, h.vcycle(y))) < 1e-10 * (1.0 + std::abs(dot(h.vcycle(x), y))), "V-cycle symmetry: " + std::to_string(dot(h.vcycle(x), y)) + " vs " + std::to_string(dot(x, h.vcycle(y))));
      expect(dot(h.vcycle(x), x) > 0.0, "V-cycle positivity");
      expect(h.levels() >= 2, "hierarchy has coarse levels");
      for (std::size_t l = 0; l + 1 < h.levels(); ++l) {
        std::vector<mg::Axis> coarse = axes;
        for (std::size_t k = 0; k <= l; ++k)
          for (auto &a : coarse) a.cells /= 2;
        std::vector<mg::Axis> fine = axes;
        for (std::size_t k = 0; k < l; ++k)
          for (auto &a : fine) a.cells /= 2;
        const Series c = noise(shape_of(coarse), 50 + l), f = noise(shape_of(fine), 60 + l);
        const double lhs = dot(h.prolong_from_coarse(l, c), f), rhs = dot(c, h.restrict_to_coarse(l, f)) * std::pow(2.0, static_cast<double>(axes.size()));
        expect(std::abs(lhs - rhs) < 1e-10 * (1.0 + std::abs(lhs)), "restriction is the adjoint of prolongation (level " + std::to_string(l) + ")");
      }
    }
    // ---- solves vs plain CG; mesh-independent iterations
    {
      std::int64_t prev = 0;
      for (std::int64_t n : {32, 64, 128, 256}) {
        std::vector<mg::Axis> axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {n, 1.0 / n, mg::Edge::Neumann, mg::Edge::Neumann}};
        const Series rhs = noise({n, n}, 5);
        mg::Hierarchy h({.axes = axes}, rhs);
        krylov::CgOptions o;
        o.relative_tolerance = 1e-10;
        o.max_iterations = 50;
        o.check_interval = 1;
        const auto r = mg::solve(h, rhs, 2, o);
        expect(r.converged.all().item<bool>(), "MG-PCG converged at n=" + std::to_string(n));
        expect(maxdiff(h.apply(r.solution), rhs) < 1e-8 * rhs.abs().max().item<double>() * 100, "MG-PCG residual");
        expect(r.iterations <= 16, "iteration count at n=" + std::to_string(n) + ": " + std::to_string(r.iterations));
        expect(prev == 0 || r.iterations <= prev + 2, "iterations grow with n: " + std::to_string(prev) + " -> " + std::to_string(r.iterations));
        prev = r.iterations;
        std::cout << "  n=" << n << " levels=" << h.levels() << " iterations=" << r.iterations << "\n";
      }
      // agreement with unpreconditioned CG on a small problem
      std::vector<mg::Axis> axes = {{16, 0.1, mg::Edge::Dirichlet, mg::Edge::Neumann}, {12, 0.1, mg::Edge::Periodic, mg::Edge::Periodic}};
      const Series rhs = noise({16, 12}, 6), coef = noise({16, 12}, 7).abs().add(0.2);
      mg::Hierarchy h({.axes = axes, .face_high = {coef, coef}}, rhs);
      const auto fast = mg::solve(h, rhs, 2);
      krylov::CgOptions slow;
      slow.max_iterations = 4000;
      slow.relative_tolerance = 1e-13;
      const auto ref = krylov::cg([&](const Series &u) { return h.apply(u); }, rhs, {0, 1}, slow);
      expect(maxdiff(fast.solution, ref.solution) < 1e-7, "MG-PCG vs plain CG");
    }
    // ---- jumping coefficients (1e3 contrast, random checkerboard blocks) stay robust
    {
      const std::int64_t n = 128;
      std::vector<double> c(static_cast<std::size_t>(n * n));
      for (std::int64_t i = 0; i < n; ++i)
        for (std::int64_t j = 0; j < n; ++j)
          c[static_cast<std::size_t>(i * n + j)] = (((i / 16) + (j / 16)) % 2) ? 1000.0 : 1.0;
      const Series coef = pluss::series::make(c, {n, n}).to(F64);
      std::vector<mg::Axis> axes = {{n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {n, 1.0 / n, mg::Edge::Dirichlet, mg::Edge::Dirichlet}};
      const Series rhs = noise({n, n}, 8);
      mg::Hierarchy h({.axes = axes, .face_high = {coef, coef}}, rhs);
      krylov::CgOptions o;
      o.relative_tolerance = 1e-9;
      o.max_iterations = 100;
      o.check_interval = 1;
      const auto r = mg::solve(h, rhs, 2, o);
      std::cout << "  contrast residual " << r.relative_residual.max().item<double>() << " its " << r.iterations << "\n";
      expect(r.converged.all().item<bool>(), "jump-coefficient MG-PCG converged");
      std::cout << "  contrast 1e3 iterations=" << r.iterations << "\n";
      expect(r.iterations <= 60, "jump-coefficient iterations: " + std::to_string(r.iterations));
    }
    // ---- singular (periodic) problem and batch lanes with their own coefficients
    {
      std::vector<mg::Axis> axes = {{32, 0.2, mg::Edge::Periodic, mg::Edge::Periodic}, {32, 0.2, mg::Edge::Periodic, mg::Edge::Periodic}};
      Series rhs = noise({3, 32, 32}, 11);
      rhs = rhs.sub(rhs.mean(std::vector<std::int64_t>{1, 2}, true));
      const Series coef = noise({3, 32, 32}, 12).abs().add(0.3);
      mg::Hierarchy h({.axes = axes, .face_high = {coef, coef}}, rhs);
      expect(h.singular(), "periodic problem is singular");
      const auto batched = mg::solve(h, rhs, 2);
      expect(batched.converged.all().item<bool>(), "batched singular solve converged");
      for (std::int64_t lane = 0; lane < 3; ++lane) {
        const Series r1 = rhs.select(0, lane), c1 = coef.select(0, lane);
        mg::Hierarchy h1({.axes = axes, .face_high = {c1, c1}}, r1);
        expect(maxdiff(mg::solve(h1, r1, 2).solution, batched.solution.select(0, lane)) < 1e-6, "lane solve vs batched solve");
      }
    }
    std::cout << "multigrid tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
