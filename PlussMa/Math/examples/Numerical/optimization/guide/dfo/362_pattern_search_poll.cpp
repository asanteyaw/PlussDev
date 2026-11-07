// The poll. Each iteration pattern search evaluates points around the best one: the 2n points +-mesh along a basis
// (PositiveBasis2N) or the n+1 points of a minimal positive basis (PositiveBasisNp1, fewer evaluations per poll).
// `polling_order` chooses the order they are tried in (Consecutive, Random, SuccessFirst = start with the direction that
// last worked), and `complete_poll` evaluates all of them even after one improves (more work, a better step).
// Same problem and start for every row; the counts are evaluations.
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::vector<std::pair<const char*, dfo::PSConfig>> cases;
  dfo::PSConfig base;
  cases.emplace_back("2N, consecutive (default)", base);
  auto np1 = base; np1.poll_pattern = dfo::PSPollPattern::PositiveBasisNp1;
  cases.emplace_back("N+1 basis", np1);
  auto rnd = base; rnd.polling_order = dfo::PSPollingOrder::Random;
  cases.emplace_back("random order", rnd);
  auto sf = base; sf.polling_order = dfo::PSPollingOrder::SuccessFirst;
  cases.emplace_back("success-first order", sf);
  auto full = base; full.complete_poll = true;
  cases.emplace_back("complete poll", full);
  auto canon = base; canon.basis_family = dfo::PSBasisFamily::Canonical;
  cases.emplace_back("coordinate-axis basis", canon);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.search_strategy = dfo::PSSearchStrategy::Off;  // isolate the poll
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
