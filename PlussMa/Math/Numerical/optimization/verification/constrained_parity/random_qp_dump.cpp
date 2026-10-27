#include <iostream>
#include <random>
#include <pluss/ma.h>
#include <iomanip>
#include <cmath>
#include <fstream>
namespace optim = pluss::subsp::math::optim;
auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {(int64_t)v.size()}).to(f64); }
pluss::Series mat(std::vector<double> v, int64_t r, int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }
int main(int argc, char** argv) {
  int N = argc > 1 ? std::atoi(argv[1]) : 100;
  std::mt19937 rng(12345);
  std::normal_distribution<double> g(0, 1);
  std::uniform_real_distribution<double> u(0, 1);
  int bad[4] = {0,0,0,0}, solved[4] = {0,0,0,0}; double worst[4] = {0,0,0,0};
  int shown = 0;
  for (int trial = 0; trial < N; ++trial) {
    int n = 2 + trial % 6;
    int meq = trial % 3;         // 0..2 equalities
    int mineq = (trial / 3) % 4; // 0..3 inequalities
    bool use_bounds = (trial / 12) % 2 == 0;
    std::vector<double> M(n*n), Q(n*n, 0.0), c(n), x0(n);
    for (auto& v : M) v = g(rng);
    for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) { double s = 0; for (int k = 0; k < n; ++k) s += M[k*n+i]*M[k*n+j]; Q[i*n+j] = s + (i==j?0.3:0.0); }
    for (auto& v : c) v = 2*g(rng);
    for (auto& v : x0) v = g(rng);
    std::vector<double> Ae(meq*n), be(meq), Ai(mineq*n), bi(mineq);
    for (int r = 0; r < meq; ++r) { double s = 0; for (int j = 0; j < n; ++j) { Ae[r*n+j] = g(rng); s += Ae[r*n+j]*x0[j]; } be[r] = s; }
    for (int r = 0; r < mineq; ++r) { double s = 0; for (int j = 0; j < n; ++j) { Ai[r*n+j] = g(rng); s += Ai[r*n+j]*x0[j]; } bi[r] = s + 0.2 + u(rng); }
    optim::ConstrainedProblem p;
    p.objective.analytic = [&](const optim::ParameterPack<pluss::Series>& pk) {
      auto x = pk.at("x").host<double>();
      double val = 0; std::vector<double> grad(n, 0.0);
      for (int i = 0; i < n; ++i) { double qx = 0; for (int j = 0; j < n; ++j) qx += Q[i*n+j]*x[j]; val += 0.5*x[i]*qx + c[i]*x[i]; grad[i] = qx + c[i]; }
      return std::pair{pluss::make_series(val).to(f64), optim::pack_series(vec(grad))};
    };
    if (meq) { p.linear.Aeq = mat(Ae, meq, n); p.linear.beq = vec(be); }
    if (mineq) { p.linear.Aineq = mat(Ai, mineq, n); p.linear.bineq = vec(bi); }
    if (use_bounds) { std::vector<double> lo(n), hi(n); for (int j = 0; j < n; ++j) { lo[j] = x0[j]-1.5-u(rng); hi[j] = x0[j]+1.5+u(rng); } p.bounds.lower = optim::pack_series(vec(lo)); p.bounds.upper = optim::pack_series(vec(hi)); }
    std::vector<double> start(n); for (int j = 0; j < n; ++j) start[j] = x0[j] + 0.5*g(rng);
    { std::ofstream f("/tmp/g/qp/prob_" + std::to_string(trial) + ".txt"); f << std::setprecision(17);
      f << n << " " << meq << " " << mineq << " " << (use_bounds ? 1 : 0) << "\n";
      for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) f << Q[i*n+j] << " "; f << "\n"; }
      for (double v : c) f << v << " "; f << "\n"; for (double v : start) f << v << " "; f << "\n";
      for (int r = 0; r < meq; ++r) { for (int j = 0; j < n; ++j) f << Ae[r*n+j] << " "; f << "\n"; } for (double v : be) f << v << " "; f << "\n";
      for (int r = 0; r < mineq; ++r) { for (int j = 0; j < n; ++j) f << Ai[r*n+j] << " "; f << "\n"; } for (double v : bi) f << v << " "; f << "\n";
      if (use_bounds) { auto lo = p.bounds.lower->at("x").host<double>(); auto hi = p.bounds.upper->at("x").host<double>(); for (double v : lo) f << v << " "; f << "\n"; for (double v : hi) f << v << " "; f << "\n"; } }
    double fref = 0; bool have_ref = false;
    const optim::OptimaAlgorithm algs[4] = {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol, optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::MeritMethod};
    double fv[4]; int flag[4]; double viol[4];
    for (int a = 0; a < 4; ++a) {
      optim::OptimaOptions o; o.algorithm = algs[a];
      auto r = optim::optima(p, optim::pack_series(vec(start)), o);
      fv[a] = r.evaluation.value.item<double>(); flag[a] = (int)r.exit_flag; viol[a] = r.evaluation.max_constraint_violation;
    }
    { std::ofstream f("/tmp/g/qp/ours_" + std::to_string(trial) + ".txt"); f << std::setprecision(17); for (int a = 0; a < 4; ++a) f << fv[a] << " " << flag[a] << " " << viol[a] << "\n"; }
    // reference: best feasible objective among all algorithms that report converged with tiny violation
    for (int a = 0; a < 4; ++a) if (flag[a] == 1 && viol[a] < 1e-5) { if (!have_ref || fv[a] < fref) { fref = fv[a]; have_ref = true; } }
    for (int a = 0; a < 4; ++a) {
      double err = have_ref ? (fv[a] - fref) : 1e9;
      bool ok = flag[a] == 1 && viol[a] < 1e-5 && std::abs(err) < 1e-4*(1+std::abs(fref));
      if (ok) ++solved[a]; else { ++bad[a]; if (a == 1 && shown < 12) { ++shown; std::cout << "IP fail trial " << trial << " n=" << n << " meq=" << meq << " mineq=" << mineq << " bounds=" << use_bounds << " flag=" << flag[a] << " f=" << fv[a] << " ref=" << fref << " viol=" << viol[a] << "\n"; } }
    }
  }
  const char* names[4] = {"SQP","IP","WS","MERIT"};
  for (int a = 0; a < 4; ++a) std::cout << names[a] << ": solved " << solved[a] << "/" << N << "\n";
}
