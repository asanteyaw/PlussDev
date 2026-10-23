// The poll. Each iteration pattern search evaluates points around the best one: the 2n points +-mesh along a basis
// (PositiveBasis2N) or the n+1 points of a minimal positive basis (PositiveBasisNp1, fewer evaluations per poll).
// `polling_order` chooses the order they are tried in (Consecutive, Random, SuccessFirst = start with the direction that
// last worked), and `complete_poll` evaluates all of them even after one improves (more work, a better step).
// Same problem and start for every row; the counts are evaluations.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

void run(const char* label, dfo::PSConfig c) {
  c.search_strategy = dfo::PSSearchStrategy::Off;  // isolate the poll
  const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
  std::cout << std::left << std::setw(36) << label << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
            << std::setw(11) << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::PSConfig base;
  run("2N, consecutive (default)", base);
  auto np1 = base; np1.poll_pattern = dfo::PSPollPattern::PositiveBasisNp1;
  run("N+1 basis", np1);
  auto rnd = base; rnd.polling_order = dfo::PSPollingOrder::Random;
  run("random order", rnd);
  auto sf = base; sf.polling_order = dfo::PSPollingOrder::SuccessFirst;
  run("success-first order", sf);
  auto full = base; full.complete_poll = true;
  run("complete poll", full);
  auto canon = base; canon.basis_family = dfo::PSBasisFamily::Canonical;
  run("coordinate-axis basis", canon);
}
