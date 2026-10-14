// Continuation / bifurcation detection checked against closed-form bifurcation points.
#include "pluss/subsp/natsci/common/continuation.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace cont = pluss::subsp::natsci::common::continuation;
using pluss::Series;

namespace {
const auto F64 = pluss::ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(F64); }
Series cols(const std::vector<Series> &c) {
  std::vector<Series> tail;
  for (std::size_t i = 1; i < c.size(); ++i)
    tail.push_back(c[i].unsqueeze(-1));
  return c[0].unsqueeze(-1).unite(tail, -1);
}
const cont::SpecialPoint *find(const cont::Branch &b, cont::PointKind k, int nth = 0) {
  for (const auto &s : b.special)
    if (s.kind == k && nth-- == 0)
      return &s;
  return nullptr;
}
int count(const cont::Branch &b, cont::PointKind k) {
  int n = 0;
  for (const auto &s : b.special)
    n += s.kind == k;
  return n;
}
} // namespace

int main() {
  try {
    const double r3 = std::sqrt(3.0);
    // fold: G = u^3 - u - p, folds at u = -+1/sqrt3, p = +-2/(3 sqrt3)
    {
      cont::Residual g = [](const Series &u, const Series &p) { return u.mul(u).mul(u).sub(u).sub(p.unsqueeze(-1)); };
      cont::Options o;
      o.p_min = -1.0; o.p_max = 1.0;
      const auto b = cont::continue_branch(g, vec({-1.2}), -1.728 + 1.2, o, 1.0);
      expect(count(b, cont::PointKind::Fold) == 2, "fold: expected two folds, got " + std::to_string(count(b, cont::PointKind::Fold)));
      const auto *f1 = find(b, cont::PointKind::Fold, 0), *f2 = find(b, cont::PointKind::Fold, 1);
      expect(std::abs(f1->p - 2.0 / (3.0 * r3)) < 1e-8 && std::abs(f1->u[0] + 1.0 / r3) < 1e-7, "fold 1 location");
      expect(std::abs(f2->p + 2.0 / (3.0 * r3)) < 1e-8 && std::abs(f2->u[0] - 1.0 / r3) < 1e-7, "fold 2 location");
      expect(count(b, cont::PointKind::BranchPoint) == 0, "fold: no spurious branch point");
    }
    // pitchfork: G = p u - u^3, branch point at p = 0; branch switching onto u = +-sqrt(p)
    {
      cont::Residual g = [](const Series &u, const Series &p) { return p.unsqueeze(-1).mul(u).sub(u.mul(u).mul(u)); };
      cont::Options o;
      o.p_min = -1.0; o.p_max = 1.0;
      const auto b = cont::continue_branch(g, vec({0.0}), -0.8, o, 1.0);
      const auto *bp = find(b, cont::PointKind::BranchPoint);
      expect(bp && std::abs(bp->p) < 1e-7, "pitchfork: branch point at p = 0");
      expect(count(b, cont::PointKind::Fold) == 0, "pitchfork: no fold on the trivial branch");
      const auto dirs = cont::branch_directions(g, vec({bp->u[0]}), bp->p, {0.0, 1.0});
      expect(!dirs.empty(), "pitchfork: branch directions");
      bool found = false;
      for (const auto &d : dirs) {
        if (std::abs(d[1]) < 0.99) {
          const auto nb = cont::continue_branch(g, vec({bp->u[0] + 1e-4 * d[0]}), bp->p + 1e-4 * d[1], o, 1.0, d);
          const double pe = nb.p.select(0, nb.points - 1).item<double>(), ue = nb.u.select(0, nb.points - 1).select(0, 0).item<double>();
          expect(std::abs(ue * ue - pe) < 1e-7, "pitchfork: new branch satisfies u^2 = p");
          found = true;
        }
      }
      expect(found, "pitchfork: a non-trivial branch direction was found");
    }
    // transcritical: G = p u - u^2 -> second branch u = p
    {
      cont::Residual g = [](const Series &u, const Series &p) { return p.unsqueeze(-1).mul(u).sub(u.mul(u)); };
      cont::Options o;
      o.p_min = -1.0; o.p_max = 1.0;
      const auto b = cont::continue_branch(g, vec({0.0}), -0.8, o, 1.0);
      const auto *bp = find(b, cont::PointKind::BranchPoint);
      expect(bp && std::abs(bp->p) < 1e-7, "transcritical: branch point at p = 0");
    }
    // Brusselator, A = 1: u' = A - (B+1) u + u^2 v, v' = B u - u^2 v; steady state (A, B/A), Hopf at B = 1 + A^2, omega = A
    {
      const double A = 1.0;
      cont::Residual g = [A](const Series &u, const Series &p) {
        const Series x = u.select(-1, 0), y = u.select(-1, 1);
        const Series x2y = x.mul(x).mul(y);
        return cols({x.mul(p.add(1.0)).mul(-1.0).add(A).add(x2y), p.mul(x).sub(x2y)});
      };
      cont::Options o;
      o.p_min = 0.5; o.p_max = 3.5;
      const auto b = cont::continue_branch(g, vec({A, 0.5 / A}), 0.5, o, 1.0);
      const auto *h = find(b, cont::PointKind::Hopf);
      expect(h && count(b, cont::PointKind::Hopf) == 1, "brusselator: exactly one Hopf point");
      expect(std::abs(h->p - (1.0 + A * A)) < 1e-6, "brusselator: Hopf at B = 1 + A^2, got " + std::to_string(h->p));
      expect(std::abs(h->omega - A) < 1e-6, "brusselator: Hopf frequency A, got " + std::to_string(h->omega));
      expect(count(b, cont::PointKind::Fold) == 0 && count(b, cont::PointKind::BranchPoint) == 0, "brusselator: only a Hopf point");
    }
    // Lorenz: trivial branch -> pitchfork at r = 1; switching to the convective branch; Hopf at r = s(s+b+3)/(s-b-1)
    {
      const double s = 10.0, bb = 8.0 / 3.0;
      cont::Residual g = [s, bb](const Series &u, const Series &p) {
        const Series x = u.select(-1, 0), y = u.select(-1, 1), z = u.select(-1, 2);
        return cols({y.sub(x).mul(s), p.mul(x).sub(y).sub(x.mul(z)), x.mul(y).sub(z.mul(bb))});
      };
      cont::Options o;
      o.p_min = 0.2; o.p_max = 30.0; o.step = 0.1;
      const auto b = cont::continue_branch(g, vec({0.0, 0.0, 0.0}), 0.5, o, 1.0);
      const auto *bp = find(b, cont::PointKind::BranchPoint);
      expect(bp && std::abs(bp->p - 1.0) < 1e-6, "lorenz: pitchfork at r = 1");
      const auto dirs = cont::branch_directions(g, vec(bp->u), bp->p, {0, 0, 0, 1.0});
      expect(!dirs.empty(), "lorenz: branch directions");
      bool done = false;
      for (const auto &d : dirs) {
        if (std::abs(d[3]) > 0.99)
          continue;
        const double sg = d[0] > 0 ? 1.0 : -1.0;
        const auto nb = cont::continue_branch(g, vec({bp->u[0] + 1e-4 * d[0], bp->u[1] + 1e-4 * d[1], bp->u[2] + 1e-4 * d[2]}), bp->p + 1e-4 * d[3], o, 1.0, d);
        const auto *h = find(nb, cont::PointKind::Hopf);
        const double rh = s * (s + bb + 3.0) / (s - bb - 1.0), wh = std::sqrt(bb * (s + rh)); // char. poly l^3 + (s+b+1) l^2 + b(s+r) l + 2 s b (r-1): roots +-i sqrt(b(s+r)) at the Hopf point
        expect(h, "lorenz: Hopf on the convective branch");
        expect(std::abs(h->p - rh) < 1e-5, "lorenz: Hopf at r = " + std::to_string(rh) + ", got " + std::to_string(h->p));
        expect(std::abs(h->omega - wh) < 1e-5, "lorenz: Hopf frequency " + std::to_string(wh) + ", got " + std::to_string(h->omega));
        (void)sg;
        done = true;
        break;
      }
      expect(done, "lorenz: convective branch followed");
    }
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  std::cout << "continuation tests passed\n";
  return 0;
}
