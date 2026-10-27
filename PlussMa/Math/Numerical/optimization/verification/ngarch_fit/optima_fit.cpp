// Fits NGARCH, component NGARCH and the RNN-driven TVP NGARCH (Gaussian returns likelihood) with Optima,
// on the scaled parameters, bounds and starts used by matlab_fit.m.
// usage: optima_fit MODEL RETURNS.csv STARTS.csv ALGORITHM [autodiff|dual]
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <pluss/ma.h>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<Series>;
using Series = optim::Trace;
using Series = pluss::Series;
static const auto f64 = pluss::ScalarType::Float64;


template <int N> using Dual = optim::Fwd<N>;
template <int N> Dual<N> dsqrt(const Dual<N>& a) { return a.sqrt(); }
template <int N> Dual<N> dlog(const Dual<N>& a) { return a.log(); }
template <int N> Dual<N> dtanh(const Dual<N>& a) { return a.tanh(); }
template <int N> Dual<N> dabs(const Dual<N>& a) { return a.abs(); }

// Gaussian returns negative log-likelihood and its gradient with respect to the unscaled parameters.
template <int N> double nll_dual(const std::string& model, const std::vector<double>& p, const std::vector<double>& y, double var, double* grad) {
  std::vector<Dual<N>> P(N);
  for (int i = 0; i < N; ++i) P[i] = Dual<N>::variable(p[i], i);
  const double c0 = 0.5 * std::log(2 * M_PI);
  Dual<N> h(var), total(0.0), z;
  auto shock = [&](const Dual<N>& hh, double yy, const Dual<N>& la) { return (hh * Dual<N>(0.5) + Dual<N>(yy) - la * dsqrt(hh)) / dsqrt(hh); };
  auto add = [&](const Dual<N>& hh, const Dual<N>& zz) { total = total + dlog(hh) * Dual<N>(0.5) + zz * zz * Dual<N>(0.5) + Dual<N>(c0); };
  const size_t n = y.size();
  if (model == "ngarch") {
    auto &om = P[0], &al = P[1], &ph = P[2], &la = P[3], &g = P[4];
    z = shock(h, y[0], la); add(h, z);
    for (size_t j = 1; j < n; ++j) {
      h = om + ph * (h - om) + al * (h * z * z + h * z * g * Dual<N>(2.0) - h);
      if (!(h.v > 0)) return std::nan("");
      z = shock(h, y[j], la); add(h, z);
    }
  } else if (model == "cngarch") {
    auto &om = P[0], &al = P[1], &ph = P[2], &la = P[3], &g1 = P[4], &g2 = P[5], &p1 = P[6], &rho = P[7];
    Dual<N> qv = h;
    z = shock(h, y[0], la); add(h, z);
    for (size_t j = 1; j < n; ++j) {
      Dual<N> qn = om + rho * (qv - om) + p1 * h * (z * z + z * g2 * Dual<N>(2.0) - Dual<N>(1.0));
      Dual<N> hn = qn + ph * (h - qv) + al * h * (z * z + z * g1 * Dual<N>(2.0) - Dual<N>(1.0));
      qv = qn; h = hn;
      if (!(h.v > 0)) return std::nan("");
      z = shock(h, y[j], la); add(h, z);
    }
  } else {
    auto &wx1 = P[0], &wx2 = P[1], &wx3 = P[2], &wh = P[3], &bx = P[4], &bh = P[5], &fw = P[6], &fb = P[7], &al = P[8], &ph = P[9], &la = P[10], &g = P[11];
    Dual<N> om = fb, hs(0.0);
    z = shock(h, y[0], la); add(h, z);
    for (size_t j = 1; j < n; ++j) {
      hs = dtanh(wx1 * om + wx2 * Dual<N>(y[j - 1]) + wx3 * h + bx + wh * hs + bh);
      om = dabs(fw * hs + fb);
      h = om + ph * (h - om) + al * h * (z * z + z * g * Dual<N>(2.0) - Dual<N>(1.0));
      if (!(h.v > 0)) return std::nan("");
      z = shock(h, y[j], la); add(h, z);
    }
  }
  for (int i = 0; i < N; ++i) grad[i] = total.d[i];
  return total.v;
}

static std::vector<double> read_returns(const std::string& path) {
  std::ifstream f(path); std::string line; std::getline(f, line);
  std::vector<double> y;
  while (std::getline(f, line)) { auto c = line.rfind(','); if (c != std::string::npos) y.push_back(std::stod(line.substr(c + 1))); }
  return y;
}
static std::vector<std::vector<double>> read_starts(const std::string& path) {
  std::ifstream f(path); std::string line; std::vector<std::vector<double>> S;
  while (std::getline(f, line)) { std::vector<double> r; std::stringstream ss(line); std::string tok; while (std::getline(ss, tok, ',')) r.push_back(std::stod(tok)); if (!r.empty()) S.push_back(r); }
  return S;
}
static optim::ParameterPack<Series> pack(const std::vector<double>& v) {
  std::vector<optim::ParameterPack<Series>::Block> out;
  for (size_t i = 0; i < v.size(); ++i) out.push_back({"p" + std::to_string(i + 1), pluss::make_series(v[i]).to(f64).reshape({1})});
  return optim::ParameterPack<Series>(out);
}

int main(int argc, char** argv) {
  const std::string model = argv[1];
  const auto y = read_returns(argv[2]);
  const auto starts = read_starts(argv[3]);
  const std::string alg = argv[4];
  const std::string mode = argc > 5 ? argv[5] : "autodiff";
  const size_t n = y.size();
  double mean = 0; for (double v : y) mean += v; mean /= n;
  double var = 0; for (double v : y) var += (v - mean) * (v - mean); var /= (n - 1);

  std::vector<double> lb, ub;
  if (model == "ngarch") { lb = {0, 0, 0, 0, -5}; ub = {1, 1, 1, 5, 0}; }
  else if (model == "cngarch") { lb = {0, 0, 0, 0, -100, -100, 0, 0}; ub = {1, 1, 1, 5, 100, 100, 1, 1}; }
  else { lb = std::vector<double>(8, -100); ub = std::vector<double>(8, 100); for (double v : {0.0, 0.0, 0.0, -100.0}) lb.push_back(v); for (double v : {1.0, 1.0, 5.0, 100.0}) ub.push_back(v); }

  const double c0 = 0.5 * std::log(2 * M_PI);
  for (size_t s = 0; s < starts.size(); ++s) {
    const auto& p0 = starts[s];
    const size_t np = p0.size();
    std::vector<double> scaler(np), th0(np), lbs(np), ubs(np);
    for (size_t i = 0; i < np; ++i) { scaler[i] = std::pow(10.0, std::ceil(std::log10(std::abs(p0[i])))); th0[i] = p0[i] / scaler[i]; lbs[i] = lb[i] / scaler[i]; ubs[i] = ub[i] / scaler[i]; }

    optim::ConstrainedProblem prob;
    prob.objective.autodiff = [&](const Pack& q) -> Series {
      auto P = [&](size_t i) { return q.at("p" + std::to_string(i + 1)) * scaler[i]; };
      Series h = (pluss::make_series(var).to(f64).reshape({1}));
      Series total = h * 0.0;
      if (model == "ngarch") {
        Series om = P(0), al = P(1), ph = P(2), la = P(3), g = P(4);
        Series z = (h * 0.5 + y[0] - la * h.sqrt()) / h.sqrt();
        total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        for (size_t j = 1; j < n; ++j) {
          h = om + ph * (h - om) + al * (h * z.square() + h * z * g * 2.0 - h);
          z = (h * 0.5 + y[j] - la * h.sqrt()) / h.sqrt();
          total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        }
      } else if (model == "cngarch") {
        Series om = P(0), al = P(1), ph = P(2), la = P(3), g1 = P(4), g2 = P(5), p1 = P(6), rho = P(7);
        Series qv = h;
        Series z = (h * 0.5 + y[0] - la * h.sqrt()) / h.sqrt();
        total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        for (size_t j = 1; j < n; ++j) {
          Series qn = om + rho * (qv - om) + p1 * h * (z.square() + z * g2 * 2.0 - 1.0);
          Series hn = qn + ph * (h - qv) + al * h * (z.square() + z * g1 * 2.0 - 1.0);
          qv = qn; h = hn;
          z = (h * 0.5 + y[j] - la * h.sqrt()) / h.sqrt();
          total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        }
      } else {
        Series wx1 = P(0), wx2 = P(1), wx3 = P(2), wh = P(3), bx = P(4), bh = P(5), fw = P(6), fb = P(7), al = P(8), ph = P(9), la = P(10), g = P(11);
        Series om = fb, hs = fb * 0.0;
        Series z = (h * 0.5 + y[0] - la * h.sqrt()) / h.sqrt();
        total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        for (size_t j = 1; j < n; ++j) {
          hs = (wx1 * om + wx2 * y[j - 1] + wx3 * h + bx + wh * hs + bh).tanh();
          om = (fw * hs + fb).square().sqrt();
          h = om + ph * (h - om) + al * h * (z.square() + z * g * 2.0 - 1.0);
          z = (h * 0.5 + y[j] - la * h.sqrt()) / h.sqrt();
          total = total + (h.log() * 0.5 + z.square() * 0.5 + c0);
        }
      }
      return total;
    };
    if (mode == "dual") {
      prob.objective.autodiff = nullptr;
      prob.objective.analytic = [&](const optim::ParameterPack<Series>& q) -> std::pair<Series, optim::ParameterPack<Series>> {
        std::vector<double> p(np), g(np, 0.0);
        for (size_t i = 0; i < np; ++i) p[i] = as_f64(q.at("p" + std::to_string(i + 1))) * scaler[i];
        double f;
        if (np == 5) f = nll_dual<5>(model, p, y, var, g.data());
        else if (np == 8) f = nll_dual<8>(model, p, y, var, g.data());
        else f = nll_dual<12>(model, p, y, var, g.data());
        for (size_t i = 0; i < np; ++i) g[i] *= scaler[i];
        if (!std::isfinite(f)) for (auto& v : g) v = std::nan("");
        return {pluss::make_series(f).to(f64), pack(g)};
      };
    }
    prob.bounds.lower = pack(lbs); prob.bounds.upper = pack(ubs);

    optim::OptimaOptions o;
    o.algorithm = alg == "sqp" ? optim::OptimaAlgorithm::SequentialQP
                : alg == "merit" ? optim::OptimaAlgorithm::MeritMethod
                : alg == "working-set" ? optim::OptimaAlgorithm::WorkingSet
                : optim::OptimaAlgorithm::InternalProtocol;
    o.sequential_qp.max_iterations = 1000; o.working_set.max_iterations = 1000;
    o.internal_protocol.max_iterations = 1000; o.penalty.max_iterations = 1000;
    const auto t0 = std::chrono::steady_clock::now();
    try {
      auto r = optim::optima(prob, pack(th0), o);
      const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      std::ostringstream ps; ps << std::setprecision(8) << "[";
      for (size_t i = 0; i < np; ++i) ps << (i ? " " : "") << as_f64(r.params.at("p" + std::to_string(i + 1))) * scaler[i];
      ps << "]";
      std::string m = r.message; m = m.substr(0, m.find('\n')).substr(0, 50);
      std::cout << model << "|" << s + 1 << "|" << (int)r.exit_flag << "|" << r.iteration << "|" << r.function_evaluations << "|"
                << std::setprecision(10) << std::fixed << r.evaluation.value.item<double>() << "|" << std::setprecision(3) << el << "|" << ps.str() << "|" << m << std::endl;
    } catch (const std::exception& e) {
      std::string m = e.what(); m = m.substr(0, m.find('\n')).substr(0, 60);
      std::cout << model << "|" << s + 1 << "|ERR|0|0|NaN|0|-|" << m << std::endl;
    }
  }
}
