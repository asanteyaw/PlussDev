// LES closures: Smagorinsky / WALE against explicit tensor formulas, limiting flows (pure shear, rotation, pure strain), the discrete energy budget of the
// coupled step, the dynamic procedure against an independent scalar-loop implementation, bounded cavity, and rejected configurations.
#include "pluss/ma/natsci/physics.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

namespace common = pluss::subsp::natsci::common;
namespace spatial = common::spatial;
namespace fluids = pluss::subsp::natsci::physics::fluids;
namespace les = fluids::les;
using pluss::ScalarType;
using pluss::Series;

namespace {
constexpr auto F64 = ScalarType::Float64;
void expect(bool c, const std::string &m) {
  if (!c)
    throw std::runtime_error(m);
}
auto metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
auto seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }
double maxabs(const Series &a) { return a.abs().max().item<double>(); }
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
using Tensor = std::vector<std::vector<Series>>;
Tensor constant_tensor(const std::vector<std::vector<double>> &m) {
  Tensor g(m.size(), std::vector<Series>(m.size()));
  for (std::size_t i = 0; i < m.size(); ++i)
    for (std::size_t j = 0; j < m.size(); ++j)
      g[i][j] = pluss::series::full({1}, m[i][j], pluss::Config{}.with_dtype(F64));
  return g;
}
// explicit-formula oracles on a 3x3 matrix
double smag_ref(const double g[3][3], double delta, double cs) {
  double s2 = 0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      const double s = 0.5 * (g[i][j] + g[j][i]);
      s2 += s * s;
    }
  return cs * cs * delta * delta * std::sqrt(2.0 * s2);
}
double wale_ref(const double g[3][3], double delta, double cw) {
  double g2[3][3] = {}, tr = 0, s2 = 0, sd2 = 0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k)
        g2[i][j] += g[i][k] * g[k][j];
  for (int k = 0; k < 3; ++k)
    tr += g2[k][k];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      const double s = 0.5 * (g[i][j] + g[j][i]);
      s2 += s * s;
      const double sd = 0.5 * (g2[i][j] + g2[j][i]) - (i == j ? tr / 3.0 : 0.0);
      sd2 += sd * sd;
    }
  const double den = std::pow(s2, 2.5) + std::pow(sd2, 1.25);
  return den > 0 ? cw * cw * delta * delta * std::pow(sd2, 1.5) / den : 0.0;
}
} // namespace

int main() {
  try {
    // ---- tensor formulas vs explicit loops on random 3-D gradients
    {
      const std::int64_t n = 64;
      Tensor g(3, std::vector<Series>(3));
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
          g[i][j] = noise({n}, 10 + 3 * i + j);
      const auto s_field = les::detail::smagorinsky(g, 0.3, 0.17), w_field = les::detail::wale(g, 0.3, 0.325);
      const auto sv = s_field.host<double>(), wv = w_field.host<double>();
      double es = 0, ew = 0;
      for (std::int64_t p = 0; p < n; ++p) {
        double m[3][3];
        for (int i = 0; i < 3; ++i)
          for (int j = 0; j < 3; ++j)
            m[i][j] = g[i][j].host<double>()[static_cast<std::size_t>(p)];
        es = std::max(es, std::abs(sv[p] - smag_ref(m, 0.3, 0.17)));
        ew = std::max(ew, std::abs(wv[p] - wale_ref(m, 0.3, 0.325)));
      }
      expect(es < 1e-14 && ew < 1e-14, "Smagorinsky / WALE vs explicit formulas: " + std::to_string(es) + " " + std::to_string(ew));
    }
    // ---- limiting flows
    {
      const double gamma = 2.0, delta = 0.1;
      const auto shear = constant_tensor({{0, gamma}, {0, 0}});
      expect(maxabs(les::detail::wale(shear, delta, 0.325)) == 0.0, "WALE vanishes in pure shear");
      expect(std::abs(les::detail::smagorinsky(shear, delta, 0.17).host<double>().front() - 0.17 * 0.17 * delta * delta * gamma) < 1e-15, "Smagorinsky in pure shear is (Cs Delta)^2 gamma");
      const auto rotation = constant_tensor({{0, -1.5}, {1.5, 0}});
      expect(maxabs(les::detail::smagorinsky(rotation, delta, 0.17)) == 0.0, "solid-body rotation produces no Smagorinsky viscosity");
      // WALE does respond to rotation (S^d_ij S^d_ij contains the Omega^4 term); with the 3-D definition's 1/3 it is the explicit formula even in 2-D
      const double sd2_rot = 2.0 * std::pow(1.5 * 1.5 / 3.0, 2); // S^d = -(w^2/3) I with two components, S_ij = 0
      expect(std::abs(les::detail::wale(rotation, delta, 0.325).host<double>().front() - std::pow(0.325 * delta, 2) * std::pow(sd2_rot, 0.25)) < 1e-15, "WALE in 2-D rotation follows the formula");
      const double a = 1.3;
      const auto strain = constant_tensor({{a, 0}, {0, -a}});
      const double sd2 = 2.0 * std::pow(a * a / 3.0, 2), s2 = 2.0 * a * a;
      const double expected = std::pow(0.325 * delta, 2) * std::pow(sd2, 1.5) / (std::pow(s2, 2.5) + std::pow(sd2, 1.25));
      expect(std::abs(les::detail::wale(strain, delta, 0.325).host<double>().front() - expected) < 1e-15, "WALE in 2-D pure strain");
    }
    // ---- Taylor-Green on a periodic grid: discrete |S| against the analytic 2|cos x cos y|, scaling with resolution
    const auto make_tg = [&](std::int64_t n, double amplitude) {
      const double h = 2.0 * std::numbers::pi / static_cast<double>(n);
      const auto grid = spatial::make_periodic_grid({n, n}, {metres(h), metres(h)});
      std::vector<double> u(static_cast<std::size_t>(2 * n * n));
      for (std::int64_t i = 0; i < n; ++i)
        for (std::int64_t j = 0; j < n; ++j) {
          const double x = (static_cast<double>(i) + 0.5) * h, y = (static_cast<double>(j) + 0.5) * h;
          u[static_cast<std::size_t>(i * n + j)] = amplitude * std::sin(x) * std::cos(y);
          u[static_cast<std::size_t>(n * n + i * n + j)] = -amplitude * std::cos(x) * std::sin(y);
        }
      auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(pluss::series::make(u, {2, n, n}).to(F64)),
                                      common::SeriesQuantity<common::Pressure>::from_si(pluss::series::zeros({n, n}, F64)), grid);
      return std::pair{grid, state};
    };
    {
      double last = 1.0;
      for (std::int64_t n : {16, 32, 64}) {
        auto [grid, state] = make_tg(n, 1.0);
        les::Closure c{.model = les::Model::Smagorinsky, .constant = 0.17};
        const Series nut = les::eddy_viscosity(state, grid, c).si_value();
        const double h = 2.0 * std::numbers::pi / static_cast<double>(n);
        std::vector<double> ref(static_cast<std::size_t>(n * n));
        for (std::int64_t i = 0; i < n; ++i)
          for (std::int64_t j = 0; j < n; ++j) {
            const double x = (static_cast<double>(i) + 0.5) * h, y = (static_cast<double>(j) + 0.5) * h;
            ref[static_cast<std::size_t>(i * n + j)] = 0.17 * 0.17 * h * h * 2.0 * std::abs(std::cos(x) * std::cos(y));
          }
        const double err = maxabs(nut.sub(pluss::series::make(ref, {n, n}).to(F64))) / (0.17 * 0.17 * h * h * 2.0);
        expect(err < last * 0.4 || n == 16, "Taylor-Green |S| error does not shrink second order: " + std::to_string(err));
        last = err;
      }
    }
    // ---- energy budget of the coupled step: d<KE>/dt(with closure) - d<KE>/dt(without) = -<nu_t |S|^2>
    {
      for (auto model : {les::Model::Smagorinsky, les::Model::Wale, les::Model::DynamicSmagorinsky}) {
        const std::int64_t n = 32;
        auto [grid, state] = make_tg(n, 1.0);
        // perturb with a divergence-free small-scale field so the dynamic model has something to see
        const Series psi = noise({n, n}, 21).mul(0.02);
        const Series pg = spatial::gradient(psi, grid);
        const Series swirl = pg.select(0, 1).unsqueeze(0).unite({pg.select(0, 0).neg().unsqueeze(0)}, 0);
        state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(state.velocity().si_value().add(swirl)), state.pressure(), grid);
        const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
        const auto nu = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(0.0);
        fluids::StepOptions opt;
        opt.rule = fluids::StepRule::Rk3;
        opt.guard.enforce = false;
        opt.projection.poisson.mean_tolerance = 1e-8;
        opt.projection.poisson.solve.relative_tolerance = 1e-13;
        opt.projection.poisson.solve.absolute_tolerance = 1e-14;
        const double dt = 1e-4;
        // project the initial data once (RK order needs a divergence-free start)
        state = fluids::project(state, grid, rho, seconds(1.0), opt.projection).state;
        les::Closure on{.model = model}, off{.model = les::Model::Smagorinsky, .constant = 0.0};
        const auto kinetic = [](const fluids::IncompressibleState &s) { return s.velocity().si_value().square().sum().mul(0.5).item<double>() / static_cast<double>(s.pressure().si_value().numel()); };
        const double k0 = kinetic(state);
        const auto a = les::advance(state, grid, rho, nu, on, seconds(0.0), seconds(dt), opt);
        const auto b = les::advance(state, grid, rho, nu, off, seconds(0.0), seconds(dt), opt);
        const double measured = (kinetic(a.state) - kinetic(b.state)) / dt;
        const double predicted = -les::sgs_dissipation(state, grid, on).host<double>().front();
        (void)k0;
        expect(std::abs(measured - predicted) < 2e-3 * std::abs(predicted), "energy budget (model " + std::to_string(static_cast<int>(model)) + "): measured " + std::to_string(measured) + " predicted " + std::to_string(predicted));
        expect(a.divergence.field.si_value().abs().max().item<double>() < 1e-8, "closure step stays divergence free");
      }
    }
    // ---- dynamic procedure vs an independent scalar-loop implementation (2-D, anisotropic spacing)
    {
      const std::int64_t n = 12, m = 10;
      const double hx = 0.3, hy = 0.4;
      const auto grid = spatial::make_periodic_grid({n, m}, {metres(hx), metres(hy)});
      Series u = noise({2, n, m}, 31);
      const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(u), common::SeriesQuantity<common::Pressure>::from_si(pluss::series::zeros({n, m}, F64)), grid);
      const auto uv = u.host<double>();
      auto U = [&](int c, std::int64_t i, std::int64_t j) { return uv[static_cast<std::size_t>(c * n * m + (((i % n) + n) % n) * m + (((j % m) + m) % m))]; };
      const double delta = std::sqrt(hx * hy), ratio = les::Closure{}.test_ratio;
      // fields on the grid
      auto grad = [&](int c, int d, std::int64_t i, std::int64_t j) { return d == 0 ? (U(c, i + 1, j) - U(c, i - 1, j)) / (2 * hx) : (U(c, i, j + 1) - U(c, i, j - 1)) / (2 * hy); };
      auto S = [&](int a, int b, std::int64_t i, std::int64_t j) { return 0.5 * (grad(a, b, i, j) + grad(b, a, i, j)); };
      auto smag = [&](std::int64_t i, std::int64_t j) { double s = 0; for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) s += S(a, b, i, j) * S(a, b, i, j); return std::sqrt(2 * s); };
      auto F = [&](auto f, std::int64_t i, std::int64_t j) { // (1/4,1/2,1/4) in each axis
        double acc = 0; const double w[3] = {0.25, 0.5, 0.25};
        for (int a = -1; a <= 1; ++a) for (int b = -1; b <= 1; ++b) acc += w[a + 1] * w[b + 1] * f(i + a, j + b);
        return acc; };
      auto Ub = [&](int c, std::int64_t i, std::int64_t j) { return F([&](std::int64_t p, std::int64_t q) { return U(c, p, q); }, i, j); };
      auto Sb = [&](int a, int b, std::int64_t i, std::int64_t j) { return F([&](std::int64_t p, std::int64_t q) { return S(a, b, p, q); }, i, j); };
      auto sbn = [&](std::int64_t i, std::int64_t j) { double s = 0; for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) s += Sb(a, b, i, j) * Sb(a, b, i, j); return std::sqrt(2 * s); };
      auto L = [&](int a, int b, std::int64_t i, std::int64_t j) { return F([&](std::int64_t p, std::int64_t q) { return U(a, p, q) * U(b, p, q); }, i, j) - Ub(a, i, j) * Ub(b, i, j); };
      auto M = [&](int a, int b, std::int64_t i, std::int64_t j) { return -2 * delta * delta * (ratio * ratio * sbn(i, j) * Sb(a, b, i, j) - F([&](std::int64_t p, std::int64_t q) { return smag(p, q) * S(a, b, p, q); }, i, j)); };
      auto Ldev = [&](int a, int b, std::int64_t i, std::int64_t j) { return L(a, b, i, j) - (a == b ? 0.5 * (L(0, 0, i, j) + L(1, 1, i, j)) : 0.0); };
      auto LM = [&](std::int64_t i, std::int64_t j) { double s = 0; for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) s += Ldev(a, b, i, j) * M(a, b, i, j); return s; };
      auto MM = [&](std::int64_t i, std::int64_t j) { double s = 0; for (int a = 0; a < 2; ++a) for (int b = 0; b < 2; ++b) s += M(a, b, i, j) * M(a, b, i, j); return s; };
      for (const bool local : {false, true}) {
        double num_g = 0, den_g = 0;
        for (std::int64_t i = 0; i < n; ++i) for (std::int64_t j = 0; j < m; ++j) { num_g += LM(i, j); den_g += MM(i, j); }
        num_g /= n * m; den_g /= n * m;
        std::vector<double> ref(static_cast<std::size_t>(n * m));
        for (std::int64_t i = 0; i < n; ++i)
          for (std::int64_t j = 0; j < m; ++j) {
            const double num = local ? F(LM, i, j) : num_g, den = local ? F(MM, i, j) : den_g;
            ref[static_cast<std::size_t>(i * m + j)] = std::max(num / den, 0.0) * delta * delta * smag(i, j);
          }
        les::Closure c{.model = les::Model::DynamicSmagorinsky, .local_average = local};
        const Series nut = les::eddy_viscosity(state, grid, c).si_value();
        expect(maxabs(nut.sub(pluss::series::make(ref, {n, m}).to(F64))) < 1e-12, std::string("dynamic Smagorinsky vs scalar loops (") + (local ? "local" : "global") + ")");
        expect(nut.ge(0.0).all().item<bool>(), "clipped dynamic viscosity is non-negative");
      }
    }
    // ---- bounded cavity and rejected configurations
    {
      const std::int64_t n = 24;
      const auto grid = spatial::make_box_grid({n, n}, {metres(1.0 / n), metres(1.0 / n)});
      const auto lid = common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({1.0, 0.0}).to(F64));
      const auto bounds = fluids::flow_bounds(grid, {fluids::no_slip({0, spatial::Side::Low}), fluids::no_slip({0, spatial::Side::High}), fluids::no_slip({1, spatial::Side::Low}), fluids::no_slip({1, spatial::Side::High}, lid)});
      auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(noise({2, n, n}, 41).mul(0.05)), common::SeriesQuantity<common::Pressure>::from_si(pluss::series::zeros({n, n}, F64)), grid);
      const auto rho = common::ScalarQuantity<fluids::Density>::from_si(1.0);
      const auto nu = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
      fluids::StepOptions opt;
      opt.guard.enforce = false;
      for (auto model : {les::Model::Smagorinsky, les::Model::Wale}) {
        const Series nut = les::eddy_viscosity(state, grid, bounds, les::Closure{.model = model}).si_value();
        expect(nut.ge(0.0).all().item<bool>() && nut.isfinite().all().item<bool>() && nut.max().item<double>() > 0.0, "bounded eddy viscosity is finite and non-negative");
        const auto next = les::advance(state, grid, bounds, rho, nu, les::Closure{.model = model}, seconds(0.0), seconds(1e-3), opt);
        expect(next.state.velocity().si_value().isfinite().all().item<bool>(), "bounded closure step finite");
      }
      bool threw = false;
      try {
        (void)les::eddy_viscosity(state, grid, bounds, les::Closure{.model = les::Model::DynamicSmagorinsky});
      } catch (const std::invalid_argument &) { threw = true; }
      expect(threw, "dynamic model rejected on bounded grids");
      threw = false;
      try {
        (void)les::eddy_viscosity(state, grid, bounds, les::Closure{.width = -1.0});
      } catch (const std::invalid_argument &) { threw = true; }
      expect(threw, "negative filter width rejected");
    }
    std::cout << "les tests passed\n";
  } catch (const std::exception &e) {
    std::cerr << "FAILED: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
